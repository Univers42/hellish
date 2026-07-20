#!/usr/bin/env python3
"""Turn the hyperfine JSON exports in bench/.artifacts/perf/ into
bench/results.md.

The headline is a single hellish-centric SCOREBOARD: one row per benchmark,
every ratio expressed relative to hellish (>1 means hellish is faster), a
consistent winner column, and a reliability flag.  This replaces hyperfine's
own per-run summaries, which pick whichever shell happens to be fastest as
the reference and so read differently every dimension.

Detail tables (median/stddev/CV per shell) follow for anyone who wants them.
"""
import json
import os
import platform

BENCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(BENCH, '.artifacts', 'perf')
OUT = os.path.join(BENCH, 'results.md')

# (benchmark key, human title, one-line description)
BENCHMARKS = [
    ('startup', 'Startup', 'fork+exec a shell, run `true`, exit'),
    ('parse50k', 'Parse 50k lines', 'lex+parse a 50k-line script under `set -n` (no execution)'),
    ('loop_arith', 'Arith loop', '100k iterations of `i=$((i+1))`'),
    ('loop_concat', 'String concat', '10k iterations of `s="${s}x"` (naive O(n^2))'),
    ('loop_colon', 'Colon loop', "100k iterations of the `:` builtin"),
    ('loop_read', 'Read loop', '`read` over a 50k-line file'),
    ('fork_cmdsub', 'Cmdsub `$(true)`', '1k `$(true)` (hellish forkless fast path)'),
    ('fork_cmdsub_ext', 'Cmdsub `$(/bin/true)`', '1k real fork+exec substitutions'),
    ('fork_pipeline', '3-stage pipeline', '300 `printf | cat | wc -l`'),
    ('configure', 'autoconf configure', 'GNU hello `./configure` (CONFIG_SHELL)'),
]

SHELLS = ('hellish', 'bash', 'dash')
CV_LIMIT = 0.03


def load(name):
    path = os.path.join(ART, name + '.json')
    if not os.path.exists(path):
        return None
    with open(path) as f:
        data = json.load(f)
    return {r['command']: r for r in data['results']}


def fmt_time(sec):
    if sec is None:
        return '—'
    if sec < 1.0:
        return '%.1f ms' % (sec * 1000)
    return '%.3f s' % sec


def ratio_cell(other_med, hel_med):
    """other/hellish: >1 => hellish faster.  Marked ✓ / ✗ / ≈."""
    if other_med is None or hel_med is None or hel_med == 0:
        return '—'
    r = other_med / hel_med
    if r >= 1.05:
        mark = '✓'
    elif r <= 0.95:
        mark = '✗'
    else:
        mark = '≈'
    return '%.2f× %s' % (r, mark)


def cell_cv(res):
    if not res or not res['median']:
        return 0.0
    return res['stddev'] / res['median']


def scoreboard(rows):
    """rows: list of (key, title, desc, labeled-or-None).  Returns md lines
    plus (wins, losses, ties, reliable_count)."""
    out = []
    a = out.append
    a('## Scoreboard — every ratio relative to hellish')
    a('')
    a('`vs bash` / `vs dash` = that shell\'s median ÷ hellish\'s median, so '
      '**> 1.0 means hellish is faster** (✓), < 1.0 means hellish is slower '
      '(✗), ≈ is within 5%.  `worst CV` is the largest coefficient of '
      'variation among the three runs — a row with worst CV > 3% (⚠) is too '
      'noisy on this machine to trust the ratio.')
    a('')
    a('| benchmark | hellish | vs bash | vs dash | fastest | worst CV |')
    a('|---|---|---|---|---|---|')
    wins = losses = ties = reliable = 0
    for key, title, _desc, labeled in rows:
        if not labeled or 'hellish' not in labeled:
            continue
        med = {s: labeled[s]['median'] for s in SHELLS if s in labeled}
        hel = med.get('hellish')
        worst_cv = max((cell_cv(labeled[s]) for s in labeled), default=0.0)
        cvflag = ' ⚠' if worst_cv > CV_LIMIT else ''
        fastest = min(med, key=med.get) if med else '—'
        vb = ratio_cell(med.get('bash'), hel)
        vd = ratio_cell(med.get('dash'), hel)
        if worst_cv <= CV_LIMIT:
            reliable += 1
            if fastest == 'hellish':
                wins += 1
            elif hel and med and med[fastest] < hel * 0.95:
                losses += 1
            else:
                ties += 1
        a('| %s | %s | %s | %s | %s | %.1f%%%s |' % (
            title, fmt_time(hel), vb, vd,
            '**hellish**' if fastest == 'hellish' else fastest,
            worst_cv * 100, cvflag))
    a('')
    return out, (wins, losses, ties, reliable)


def detail_tables(rows):
    out = []
    a = out.append
    a('## Per-benchmark detail')
    a('')
    a('| benchmark | shell | median | stddev | CV |')
    a('|---|---|---|---|---|')
    for _key, title, _desc, labeled in rows:
        if not labeled:
            continue
        for s in SHELLS:
            if s not in labeled:
                continue
            r = labeled[s]
            cv = cell_cv(r)
            a('| %s | %s | %s | %s | %.1f%%%s |' % (
                title, '**hellish**' if s == 'hellish' else s,
                fmt_time(r['median']), fmt_time(r['stddev']),
                cv * 100, ' ⚠' if cv > CV_LIMIT else ''))
    a('')
    return out


def main():
    gov = os.environ.get('GOVERNOR', 'unknown')
    rows = [(k, t, d, load(k)) for k, t, d in BENCHMARKS]
    board, (wins, losses, ties, reliable) = scoreboard(rows)

    lines = []
    a = lines.append
    a('# Benchmark results: hellish vs bash vs dash')
    a('')
    a('Generated by `make perf`.  Fairness choices: '
      '[METHODOLOGY.md](METHODOLOGY.md).')
    a('')
    if gov != 'performance':
        a('> ⚠ **CPU governor is `%s`, not `performance`.** Absolute times '
          'are inflated and noisy (frequency scaling swings ~5×); trust only '
          'the rows whose *worst CV* is low. For publication-grade numbers, '
          'run `sudo cpupower frequency-set -g performance` then '
          '`make perf`.' % gov)
        a('')
    a('Environment: %s.' % platform.platform())
    a('')
    if reliable:
        a('**On the %d benchmarks that were reliable here (worst CV ≤ 3%%): '
          'hellish is fastest in %d, tied in %d, slower in %d.**'
          % (reliable, wins, ties, losses))
        a('')
    lines += board
    lines += detail_tables(rows)

    skipped = os.path.join(ART, 'configure-skipped.txt')
    if os.path.exists(skipped) and os.path.getsize(skipped):
        a('## Configure completion note')
        a('')
        a('Excluded from the configure timing (no `config.status` — timing a '
          'fast failure against a real run would mislead):')
        a('')
        with open(skipped) as f:
            for line in f:
                if line.strip():
                    a('- %s' % line.strip())
        a('')
    with open(OUT, 'w') as f:
        f.write('\n'.join(lines))
    print('wrote %s' % OUT)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Normalize every harness's output into ONE dataset: bench/.artifacts/dashboard.json.

Five harnesses, five wire formats, five vocabularies for the same ideas.  Rather
than teach the chart generator all of them, everything is funnelled through here
into a single shape, so gen_charts.py only ever reads one file and a chart can
never disagree with the report it came from.

Sources (each optional -- a missing one yields a null section, never a crash,
so you can chart what you have while a slow harness is still running):

  bench/.artifacts/perf/*.json  hyperfine  -> initialization, parsing, execution
  bench/.artifacts/rss.json     run_rss.sh -> resources
  bench/.artifacts/conformance.json        -> conformance
  tests/bench_results.txt       benchmark  -> execution on real programs
  tests/agnostic_results.tsv    agnostic   -> cross-shell matrix

Reliability is carried alongside every timing, never dropped: each perf row
keeps its coefficient of variation and a `reliable` flag (worst CV <= 3%), and
the whole file records the CPU governor.  A chart that hides an unreliable
number is a lying chart, so the generator is given what it needs to mark it.
"""
import json
import os
import platform
import statistics
import sys

BENCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(BENCH)
ART = os.path.join(BENCH, '.artifacts')
OUT = os.path.join(ART, 'dashboard.json')

CV_LIMIT = 0.03
SHELLS = ('hellish', 'bash', 'dash')

# (key, title, dimension, blurb).  `dimension` is what groups a benchmark into
# one of the five charts the README shows.
BENCHMARKS = [
    ('startup',         'Startup',              'initialization',
     'fork+exec a shell, run `true`, exit'),
    ('parse50k',        'Parse 50k lines',      'parsing',
     'lex+parse a 50k-line script under `set -n` (no execution)'),
    ('loop_arith',      'Arith loop',           'execution',
     '100k iterations of `i=$((i+1))`'),
    ('loop_concat',     'String concat',        'execution',
     '10k iterations of `s="${s}x"`'),
    ('loop_colon',      'Colon loop',           'execution',
     '100k iterations of the `:` builtin'),
    ('loop_read',       'Read loop',            'execution',
     '`read` over a 50k-line file'),
    ('fork_cmdsub',     'Cmdsub `$(true)`',     'execution',
     '1k `$(true)` (hellish forkless fast path)'),
    ('fork_cmdsub_ext', 'Cmdsub `$(/bin/true)`', 'execution',
     '1k real fork+exec substitutions'),
    ('fork_pipeline',   '3-stage pipeline',     'execution',
     '300 `printf | cat | wc -l`'),
    ('configure',       'autoconf configure',   'execution',
     'GNU hello `./configure` (CONFIG_SHELL)'),
]


def read_json(path):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


# ---- perf (hyperfine) -----------------------------------------------------
def collect_perf():
    """One row per benchmark: each shell's median/stddev/CV plus ratios vs
    hellish.  Ratio is other/hellish so >1 always means hellish is faster --
    the same convention report_perf.py uses, so the charts and results.md can
    never tell opposite stories."""
    rows = []
    for key, title, dim, blurb in BENCHMARKS:
        data = read_json(os.path.join(ART, 'perf', key + '.json'))
        if not data:
            continue
        by = {r['command']: r for r in data.get('results', [])}
        shells, worst_cv = {}, 0.0
        for name in SHELLS:
            r = by.get(name)
            if not r or not r.get('median'):
                shells[name] = None
                continue
            med = r['median']
            cv = (r.get('stddev') or 0.0) / med if med else 0.0
            worst_cv = max(worst_cv, cv)
            shells[name] = {
                'median_s': med,
                'stddev_s': r.get('stddev'),
                'cv': cv,
                'runs': len(r.get('times') or []),
                'user_s': r.get('user'),
                'system_s': r.get('system'),
            }
        hel = shells.get('hellish')
        if not hel:
            continue
        ratios = {}
        for name in SHELLS:
            if name == 'hellish' or not shells.get(name):
                continue
            ratios[name] = shells[name]['median_s'] / hel['median_s']
        ranked = [(n, s['median_s']) for n, s in shells.items() if s]
        rows.append({
            'key': key, 'title': title, 'dimension': dim, 'blurb': blurb,
            'shells': shells, 'ratios': ratios,
            'fastest': min(ranked, key=lambda t: t[1])[0] if ranked else None,
            'worst_cv': worst_cv,
            'reliable': worst_cv <= CV_LIMIT,
        })
    return rows or None


# ---- resources (run_rss.sh) -----------------------------------------------
def collect_rss():
    data = read_json(os.path.join(ART, 'rss.json'))
    if not data:
        return None
    rows = []
    for w in data.get('workloads', []):
        shells = {}
        for name in SHELLS:
            s = (w.get('shells') or {}).get(name) or {}
            shells[name] = s.get('median_kib')
        hel = shells.get('hellish')
        rows.append({
            'key': w['key'], 'title': w['title'], 'shells': shells,
            'ratios': {n: (shells[n] / hel)
                       for n in SHELLS
                       if n != 'hellish' and shells.get(n) and hel},
        })
    return {'floor_kib': data.get('floor_kib'), 'rounds': data.get('rounds'),
            'workloads': rows}


# ---- conformance ----------------------------------------------------------
def collect_conformance():
    """Pass-rates are already clean JSON; the only work is flattening the two
    suites into one shape and carrying the caveat that hellish's rate is the
    conservative one (the `ok` bucket needs a per-shell annotation in the spec
    file, which exists for bash/dash but cannot exist for hellish)."""
    data = read_json(os.path.join(ART, 'conformance.json'))
    if not data:
        return None
    out = {'suites': [], 'consensus_bugs': {}}
    oils = (data.get('oils') or {}).get('shells')
    if oils:
        out['suites'].append({
            'key': 'oils', 'title': 'Oils spec tests',
            'files': (data.get('oils') or {}).get('files'),
            'shells': {n: {'good': v.get('good'), 'total': v.get('total'),
                           'rate': v.get('rate'), 'fail': v.get('fail')}
                       for n, v in oils.items()},
        })
    mksh = data.get('mksh')
    if mksh:
        out['suites'].append({
            'key': 'mksh', 'title': 'mksh check.t',
            'files': None,
            'shells': {n: {'good': v.get('pass'), 'total': v.get('total'),
                           'rate': v.get('rate'), 'fail': v.get('fail')}
                       for n, v in mksh.items()},
        })
    for suite, bugs in (data.get('consensus_bugs') or {}).items():
        out['consensus_bugs'][suite] = len(bugs)
    return out if out['suites'] else None


# ---- make bench (tests/benchmark) -----------------------------------------
def collect_vs_bash():
    """tests/bench_results.txt: `class name hellish_us bash_us ratio`, one line
    per task.  Reduced to a per-class geomean because that is the number the
    harness itself reports as the verdict."""
    path = os.path.join(ROOT, 'tests', 'bench_results.txt')
    try:
        with open(path) as f:
            lines = [l.split() for l in f if l.strip() and not l.startswith('#')]
    except OSError:
        return None
    classes, tasks = {}, []
    for parts in lines:
        if len(parts) < 5:
            continue
        cls, name, hus, bus, ratio = parts[0], parts[1], parts[2], parts[3], parts[4]
        try:
            hus, bus, ratio = int(hus), int(bus), float(ratio)
        except ValueError:
            continue
        tasks.append({'class': cls, 'name': name.replace('_', ' '),
                      'hellish_us': hus, 'bash_us': bus, 'ratio': ratio})
        classes.setdefault(cls, []).append(ratio)
    if not tasks:
        return None
    summary = {}
    import math
    for cls, rs in classes.items():
        summary[cls] = {
            'n': len(rs),
            'geomean': math.exp(sum(math.log(r) for r in rs if r > 0) / len(rs)),
            'faster': sum(1 for r in rs if r > 1.02),
            'parity': sum(1 for r in rs if 0.98 <= r <= 1.02),
            'slower': sum(1 for r in rs if r < 0.98),
        }
    allr = [t['ratio'] for t in tasks if t['ratio'] > 0]
    return {
        'tasks': tasks, 'classes': summary,
        'overall': {
            'n': len(allr),
            'geomean': math.exp(sum(math.log(r) for r in allr) / len(allr)),
            'median': statistics.median(allr),
        },
    }


# ---- agnostic (cross-shell matrix) ----------------------------------------
def collect_agnostic():
    path = os.path.join(ROOT, 'tests', 'agnostic_results.tsv')
    try:
        with open(path) as f:
            rows = [l.rstrip('\n').split('\t') for l in f
                    if l.strip() and not l.startswith('#')]
    except OSError:
        return None
    cells, rank = {}, []
    for r in rows:
        if r[0] == 'cell' and len(r) >= 4:
            try:
                cells.setdefault(r[1].replace('_', ' '), {})[r[2]] = int(r[3])
            except ValueError:
                pass
        elif r[0] == 'rank' and len(r) >= 6:
            try:
                rank.append({'place': int(r[1]), 'shell': r[2], 'n': int(r[3]),
                             'geomean_us': float(r[4]), 'wall_us': float(r[5])})
            except ValueError:
                pass
    if not rank and not cells:
        return None
    return {'cells': cells, 'ranking': sorted(rank, key=lambda d: d['place'])}


def governor():
    try:
        with open('/sys/devices/system/cpu/cpu2/cpufreq/scaling_governor') as f:
            return f.read().strip()
    except OSError:
        return 'unknown'


def main():
    gov = governor()
    data = {
        'meta': {
            'platform': platform.platform(),
            'governor': gov,
            # The single most important honesty flag in the file: on a
            # throttling CPU absolute milliseconds are not comparable across
            # runs, and the charts say so on their face.
            'governor_ok': gov == 'performance',
            'cv_limit': CV_LIMIT,
            'generated_by': 'bench/lib/collect_data.py',
        },
        'perf': collect_perf(),
        'resources': collect_rss(),
        'conformance': collect_conformance(),
        'vs_bash': collect_vs_bash(),
        'agnostic': collect_agnostic(),
    }
    os.makedirs(ART, exist_ok=True)
    with open(OUT, 'w') as f:
        json.dump(data, f, indent=2)
    have = [k for k in ('perf', 'resources', 'conformance', 'vs_bash', 'agnostic')
            if data[k]]
    missing = [k for k in ('perf', 'resources', 'conformance', 'vs_bash', 'agnostic')
               if not data[k]]
    print('dashboard.json: %s' % ', '.join(have), file=sys.stderr)
    if missing:
        print('  (no data yet: %s)' % ', '.join(missing), file=sys.stderr)
    if not data['meta']['governor_ok']:
        print('  !! governor=%s -- charts will be marked provisional' % gov,
              file=sys.stderr)


if __name__ == '__main__':
    main()

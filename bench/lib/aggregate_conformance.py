#!/usr/bin/env python3
"""Aggregate conformance results into bench/conformance.md + a JSON summary,
and enforce the regression gate against bench/baseline/conformance.json.

Inputs (produced by run_oils.sh / run_mksh.sh):
  bench/.artifacts/oils/<file>.tsv   case<TAB>shell<TAB>result rows
  bench/.artifacts/mksh-<label>.txt  check.pl output (pass/FAIL lines)

Judgement model:
  - Oils: a case counts toward pass-rate when the result is `pass` or `ok`.
    `ok` means "matched an expectation annotated for that shell" — such
    annotations exist for bash/dash but CANNOT exist for hellish (the spec
    files don't know it), so hellish's number is the conservative one.
  - "Consensus bug": hellish fails while BOTH bash --posix and dash pass —
    that is a real divergence from agreed behaviour, i.e. the repair queue.
  - Gate: hellish's per-suite pass counts must not drop vs the baseline.
"""
import json
import os
import re
import sys
from collections import defaultdict

BENCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(BENCH, '.artifacts')
OILS_ART = os.path.join(ART, 'oils')
OILS_SPEC = os.path.join(BENCH, 'suites', 'oils', 'spec')
BASELINE = os.path.join(BENCH, 'baseline', 'conformance.json')

SHELLS = ['hellish', 'bash', 'dash']
GOOD = {'pass', 'ok'}


def case_descs(spec_name):
    path = os.path.join(OILS_SPEC, spec_name + '.test.sh')
    descs = []
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                if line.startswith('#### '):
                    descs.append(line[5:].strip())
    except OSError:
        pass
    return descs


def read_oils():
    files = {}
    if not os.path.isdir(OILS_ART):
        return files
    for tsv in sorted(os.listdir(OILS_ART)):
        if not tsv.endswith('.tsv'):
            continue
        name = tsv[:-4]
        cases = defaultdict(dict)   # case_num -> {shell: result}
        with open(os.path.join(OILS_ART, tsv)) as f:
            header = f.readline()
            for line in f:
                parts = line.rstrip('\n').split('\t')
                if len(parts) < 3:
                    continue
                num, shell, result = parts[0], parts[1], parts[2].lower()
                # sh_spec labels shells by basename; keep known ones only.
                if shell in SHELLS:
                    cases[int(num)][shell] = result
        files[name] = cases
    return files


def read_mksh():
    out = {}
    for label in SHELLS:
        path = os.path.join(ART, 'mksh-%s.txt' % label)
        if not os.path.exists(path):
            continue
        passed, failed = set(), set()
        with open(path, errors='replace') as f:
            for line in f:
                m = re.match(r'^(pass|FAIL) check\.t:(\S+)', line)
                if m:
                    (passed if m.group(1) == 'pass' else failed).add(m.group(2))
        out[label] = {'passed': passed, 'failed': failed}
    return out


def main():
    update_baseline = os.environ.get('UPDATE_BASELINE') == '1'
    oils = read_oils()
    mksh = read_mksh()

    # ---- Oils aggregation -------------------------------------------------
    tally = {sh: defaultdict(int) for sh in SHELLS}
    per_file = {}
    consensus = []            # (file, case_num, desc, hellish_result)
    hellish_only_fail = 0
    for name, cases in sorted(oils.items()):
        descs = case_descs(name)
        ft = {sh: 0 for sh in SHELLS}
        for num, results in sorted(cases.items()):
            for sh in SHELLS:
                r = results.get(sh)
                if r is None:
                    continue
                tally[sh][r] += 1
                if r in GOOD:
                    ft[sh] += 1
            h, b, d = (results.get(s) for s in SHELLS)
            if h is not None and h not in GOOD:
                if b in GOOD and d in GOOD:
                    desc = descs[num] if num < len(descs) else '?'
                    consensus.append((name, num, desc, h))
                hellish_only_fail += 1
        per_file[name] = {'total': len(cases), **ft}

    oils_totals = {}
    for sh in SHELLS:
        t = tally[sh]
        total = sum(t.values())
        good = t.get('pass', 0) + t.get('ok', 0)
        oils_totals[sh] = {
            'pass': t.get('pass', 0), 'ok': t.get('ok', 0),
            'n_i': t.get('n-i', 0), 'bug': t.get('bug', 0),
            'fail': t.get('fail', 0), 'timeout': t.get('timeout', 0),
            'good': good, 'total': total,
            'rate': round(100.0 * good / total, 2) if total else 0.0,
        }

    # ---- mksh aggregation -------------------------------------------------
    mksh_totals = {}
    for label, r in mksh.items():
        total = len(r['passed']) + len(r['failed'])
        mksh_totals[label] = {
            'pass': len(r['passed']), 'fail': len(r['failed']),
            'total': total,
            'rate': round(100.0 * len(r['passed']) / total, 2) if total else 0.0,
        }
    mksh_consensus = []
    if all(l in mksh for l in SHELLS):
        mksh_consensus = sorted(
            (mksh['hellish']['failed'] & mksh['bash']['passed']
             & mksh['dash']['passed']))

    summary = {
        'oils': {'files': len(oils), 'shells': oils_totals},
        'mksh': mksh_totals,
        'consensus_bugs': {
            'oils': [{'file': f, 'case': n, 'desc': d, 'result': r}
                     for f, n, d, r in consensus],
            'mksh': mksh_consensus,
        },
    }
    os.makedirs(ART, exist_ok=True)
    with open(os.path.join(ART, 'conformance.json'), 'w') as f:
        json.dump(summary, f, indent=2)

    write_markdown(summary, per_file)

    # ---- regression gate --------------------------------------------------
    gate_keys = {
        'oils': oils_totals.get('hellish', {}).get('good', 0),
        'mksh': mksh_totals.get('hellish', {}).get('pass', 0),
    }
    status = 0
    if os.path.exists(BASELINE) and not update_baseline:
        with open(BASELINE) as f:
            base = json.load(f)
        for suite, now in gate_keys.items():
            was = base.get(suite, 0)
            if now < was:
                print('GATE: %s regressed: hellish pass count %d -> %d'
                      % (suite, was, now), file=sys.stderr)
                status = 1
            else:
                print('GATE: %s ok: %d -> %d' % (suite, was, now),
                      file=sys.stderr)
    else:
        os.makedirs(os.path.dirname(BASELINE), exist_ok=True)
        with open(BASELINE, 'w') as f:
            json.dump(gate_keys, f, indent=2)
        print('GATE: baseline written: %s' % gate_keys, file=sys.stderr)
    sys.exit(status)


def write_markdown(summary, per_file):
    lines = []
    a = lines.append
    a('# Conformance: hellish vs bash --posix vs dash')
    a('')
    a('Generated by `make conformance`.  Two independent third-party suites:')
    a('the [Oils spec tests](https://github.com/oils-for-unix/oils) '
      '(the %d spec files whose `compare_shells` includes dash) and '
      "mksh's own regression suite (`check.t`)." % summary['oils']['files'])
    a('')
    a('Scoring: pass-rate counts `pass` + `ok`.  `ok` requires a per-shell '
      'annotation in the spec file; those exist for bash/dash but cannot '
      "exist for hellish, so hellish's rate is the conservative one.")
    a('')
    a('## Oils spec tests')
    a('')
    a('| shell | pass | ok | N-I | BUG | FAIL | timeout | total | pass-rate |')
    a('|---|---|---|---|---|---|---|---|---|')
    sh_names = {'hellish': '**hellish --posix**', 'bash': 'bash --posix',
                'dash': 'dash'}
    for sh in ['hellish', 'bash', 'dash']:
        t = summary['oils']['shells'].get(sh)
        if not t:
            continue
        a('| %s | %d | %d | %d | %d | %d | %d | %d | %.2f%% |' % (
            sh_names[sh], t['pass'], t['ok'], t['n_i'], t['bug'],
            t['fail'], t['timeout'], t['total'], t['rate']))
    a('')
    bugs = summary['consensus_bugs']['oils']
    a('### Consensus divergences (hellish fails, bash AND dash pass): %d'
      % len(bugs))
    a('')
    a('These are the real bugs — behaviour bash --posix and dash agree on '
      'that hellish gets wrong.')
    a('')
    for b in bugs:
        a('- `%s` case %d [%s]: %s' % (b['file'], b['case'],
                                       b['result'].upper(), b['desc']))
    a('')
    a('### Per-file pass counts (pass+ok / cases)')
    a('')
    a('| spec file | cases | hellish | bash | dash |')
    a('|---|---|---|---|---|')
    for name, ft in sorted(per_file.items()):
        a('| %s | %d | %d | %d | %d |' % (
            name, ft['total'], ft['hellish'], ft['bash'], ft['dash']))
    a('')
    a('## mksh check.t')
    a('')
    a('| shell | pass | fail | total | pass-rate |')
    a('|---|---|---|---|---|')
    for sh in ['hellish', 'bash', 'dash']:
        t = summary['mksh'].get(sh)
        if not t:
            continue
        a('| %s | %d | %d | %d | %.2f%% |' % (
            sh_names[sh], t['pass'], t['fail'], t['total'], t['rate']))
    a('')
    mb = summary['consensus_bugs']['mksh']
    a('### Consensus divergences on check.t: %d' % len(mb))
    a('')
    for name in mb:
        a('- `check.t:%s`' % name)
    a('')
    skipped = os.path.join(OILS_ART, 'oils-skipped.txt')
    if os.path.exists(skipped):
        a('## Skipped Oils files')
        a('')
        with open(skipped) as f:
            for line in f:
                a('- %s' % line.strip())
        a('')
    with open(os.path.join(BENCH, 'conformance.md'), 'w') as f:
        f.write('\n'.join(lines))


if __name__ == '__main__':
    main()

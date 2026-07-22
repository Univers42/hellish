#!/bin/bash
# Peak-RSS dimension: how much memory each shell needs to do the same job.
#
# Speed is only half the story -- a shell that wins a loop benchmark by
# arena-allocating everything and never giving it back is not obviously
# better.  This measures the other half: peak resident set size for the SAME
# workload scripts run.sh times, so the two reports line up dimension for
# dimension.
#
# Measured with GNU time(1)'s %M (the kernel's ru_maxrss high-water mark), NOT
# by polling /proc (races a short-lived process and undercounts a spike) and
# NOT by fork+exec'ing from python: ru_maxrss is a high-water mark over the
# whole process lifetime, so a python parent donates its ~10MB image to every
# child's peak before exec() replaces it and every shell reads back an
# identical ~10MB.  GNU time is a 27KB binary, so its own floor is noise.
#
# The absolute floor is not zero -- /bin/true costs ~1.5MB of mapped libc -- so
# read these numbers as "cost to exist and run the job", the same quantity a
# user pays, rather than as heap usage.
#
# Tunables: BENCH_CPU (pin core, default 2), RSS_ROUNDS (default 7).
set -eu

BENCH="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(cd "$BENCH/.." && pwd)"
ART="$BENCH/.artifacts"
GEN="$BENCH/workloads/gen"
CPU="${BENCH_CPU:-2}"
ROUNDS="${RSS_ROUNDS:-7}"

HELLISH="$BENCH/.bin/hellish"
mkdir -p "$ART"

[ -x "$HELLISH" ] || { echo "!! $HELLISH missing; run bench/run.sh first" >&2; exit 1; }
[ -d "$GEN" ] || /bin/bash "$BENCH/lib/gen_workloads.sh"

# GNU time is the whole measurement: without it we would be guessing.
TIME_BIN=""
for cand in /usr/bin/time /usr/local/bin/time gtime; do
    p="$(command -v "$cand" 2>/dev/null)" || continue
    if "$p" -f '%M' /bin/true >/dev/null 2>&1; then TIME_BIN="$p"; break; fi
done
if [ -z "$TIME_BIN" ]; then
    echo "!! GNU time(1) not found -- skipping the RSS dimension." >&2
    echo "!! install it (apt install time / pacman -S time) and re-run." >&2
    exit 0
fi

echo "== peak RSS (${ROUNDS} rounds/workload/shell, via $TIME_BIN)" >&2

BENCH_DIR="$BENCH" GEN_DIR="$GEN" CPU="$CPU" ROUNDS="$ROUNDS" \
HELLISH_BIN="$HELLISH" TIME_BIN="$TIME_BIN" python3 - <<'PY'
import json, os, statistics, subprocess, sys, tempfile

BENCH = os.environ['BENCH_DIR']
GEN = os.environ['GEN_DIR']
CPU = os.environ['CPU']
ROUNDS = int(os.environ['ROUNDS'])
HELLISH = os.environ['HELLISH_BIN']
TIME_BIN = os.environ['TIME_BIN']

# (key, title, script).  `startup` has no script: it is the floor, the memory a
# shell costs just to exist -- what every `$(...)`, every `make` recipe line and
# every `#!/bin/sh` script pays before doing any work at all.
WORKLOADS = [
    ('startup',       'Startup',          None),
    ('parse50k',      'Parse 50k lines',  'parse50k.sh'),
    ('loop_arith',    'Arith loop',       'loop_arith.sh'),
    ('loop_concat',   'String concat',    'loop_concat.sh'),
    ('loop_read',     'Read loop',        'loop_read.sh'),
    ('fork_pipeline', '3-stage pipeline', 'fork_pipeline.sh'),
]

SHELLS = [
    ('hellish', [HELLISH, '--posix']),
    ('bash',    ['/bin/bash', '--norc', '--posix']),
    ('dash',    ['/usr/bin/dash']),
]

# A bare exec'd binary that allocates nothing: the mapped-libc floor every
# number here sits on top of.  Reported so the charts can be honest about it.
def measure(argv):
    """Peak RSS in KiB for one run of argv, or None if the run failed."""
    with tempfile.NamedTemporaryFile('r+', delete=False) as tf:
        path = tf.name
    try:
        rc = subprocess.call(
            [TIME_BIN, '-f', '%M', '-o', path] + argv,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with open(path) as f:
            txt = f.read().strip().splitlines()
        return (int(txt[-1]) if txt else None), rc
    except (ValueError, IndexError, OSError):
        return None, 1
    finally:
        os.unlink(path)


floor_samples = [measure(['taskset', '-c', CPU, '/bin/true'])[0]
                 for _ in range(ROUNDS)]
floor = statistics.median([s for s in floor_samples if s])

out = {'rounds': ROUNDS, 'unit': 'KiB', 'floor_kib': floor, 'workloads': []}
for key, title, script in WORKLOADS:
    entry = {'key': key, 'title': title, 'shells': {}}
    for name, base in SHELLS:
        argv = ['taskset', '-c', CPU] + base
        argv += ['-c', 'true'] if script is None else [os.path.join(GEN, script)]
        samples, bad = [], 0
        for _ in range(ROUNDS):
            kib, rc = measure(argv)
            if rc != 0 or kib is None:
                bad += 1
            if kib is not None:
                samples.append(kib)
        if not samples:
            entry['shells'][name] = {'median_kib': None, 'failures': bad}
            continue
        entry['shells'][name] = {
            'median_kib': statistics.median(samples),
            'min_kib': min(samples),
            'max_kib': max(samples),
            'failures': bad,
        }
    out['workloads'].append(entry)
    row = '  '.join(
        '%s %s' % (n, '%.1fMB' % (entry['shells'][n]['median_kib'] / 1024.0)
                   if entry['shells'][n]['median_kib'] else 'n/a')
        for n, _ in SHELLS)
    print('  ▸ %-18s %s' % (title, row), file=sys.stderr)

with open(os.path.join(BENCH, '.artifacts', 'rss.json'), 'w') as f:
    json.dump(out, f, indent=2)
print('  floor (/bin/true): %.1fMB' % (floor / 1024.0), file=sys.stderr)
print('  wrote .artifacts/rss.json', file=sys.stderr)
PY

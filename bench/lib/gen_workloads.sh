#!/bin/bash
# Generate the benchmark workload scripts into bench/workloads/gen/.
# Deterministic output (no randomness) so runs are comparable across time.
# Every script is strict POSIX sh — the same file is fed to every shell.
set -eu

BENCH_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GEN="$BENCH_DIR/workloads/gen"
mkdir -p "$GEN"

# --- parse-only: 50k lines of varied real-shaped code under `set -n` -------
# `set -n` (noexec) is POSIX and honored by bash, dash and hellish for
# non-interactive input: the whole file is lexed and parsed, nothing runs.
python3 - "$GEN/parse50k.sh" <<'EOF'
import sys
out = open(sys.argv[1], 'w')
out.write('set -n\n')
chunk = '''foo_%d() {
    local_var="value-%d"
    case "$local_var" in
        value-*) x=$(printf '%%s\\n' "$local_var" | sed 's/value/key/') ;;
        *) for f in a b c "$local_var"; do echo "$f" >> /dev/null; done ;;
    esac
    if [ "${x:-unset}" != unset ] && [ -n "$local_var" ]; then
        y=`expr %d + 1`
        while [ "$y" -gt 0 ]; do y=$((y - 1)); done
    elif echo "$local_var" | grep -q value; then
        z="${local_var%%%%-*}_${local_var#value-}"
    fi
}
'''
i = 0
lines = 1
while lines < 50000:
    text = chunk % (i, i, i)
    out.write(text)
    lines += text.count('\n')
    i += 1
out.close()
print('parse50k.sh: %d lines' % lines)
EOF

# --- loop throughput: 100k iterations each --------------------------------
cat > "$GEN/loop_arith.sh" <<'EOF'
i=0
while [ "$i" -lt 100000 ]; do
    i=$((i + 1))
done
echo "$i"
EOF

# String concat is O(n^2) in a naive shell (each iteration copies the whole
# growing value), so 10k iterations already runs ~1s in bash while 100k runs
# ~85s -- far too slow for 30 timed repetitions.  The cross-shell RATIO is
# scale-invariant (all shells are quadratic here), so 10k measures the same
# relative standing at 1/70th the wall time.
cat > "$GEN/loop_concat.sh" <<'EOF'
s=
i=0
while [ "$i" -lt 10000 ]; do
    s="${s}x"
    i=$((i + 1))
done
echo "${#s}"
EOF

cat > "$GEN/loop_colon.sh" <<'EOF'
i=0
while [ "$i" -lt 100000 ]; do
    :
    i=$((i + 1))
done
EOF

seq -f 'input line %06.0f with some words on it' 50000 > "$GEN/read_input.txt"
# The input path is baked in so the script needs no environment lookup and
# hyperfine can exec the shell directly with no wrapper.
cat > "$GEN/loop_read.sh" <<EOF
n=0
while read -r line; do
    n=\$((n + 1))
done < "$GEN/read_input.txt"
echo "\$n"
EOF

# --- fork workloads --------------------------------------------------------
# cmdsub with a builtin body: hellish's forkless fast path applies (the same
# trick ksh93 uses); bash and dash fork.  The external variant forces a real
# fork+exec for every shell.
cat > "$GEN/fork_cmdsub.sh" <<'EOF'
i=0
while [ "$i" -lt 1000 ]; do
    x=$(true)
    i=$((i + 1))
done
EOF

cat > "$GEN/fork_cmdsub_ext.sh" <<'EOF'
i=0
while [ "$i" -lt 1000 ]; do
    x=$(/bin/true)
    i=$((i + 1))
done
EOF

# 300 iterations of a 3-stage pipeline = 900 fork+exec + pipe setups, ~1s per
# run; 1000 pushed a single run past 3s (30x that is too slow to repeat).
cat > "$GEN/fork_pipeline.sh" <<'EOF'
i=0
while [ "$i" -lt 300 ]; do
    printf 'alpha\nbeta\ngamma\n' | cat | wc -l > /dev/null
    i=$((i + 1))
done
EOF

echo "workloads generated in $GEN" >&2

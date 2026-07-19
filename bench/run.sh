#!/bin/bash
# Dimension-split speed benchmark: hellish vs bash --norc --posix vs dash.
# See METHODOLOGY.md for every fairness choice made here.
#
#   make perf                 # strict: refuses to run without the
#                             # performance governor
#   BENCH_LAX=1 make perf     # run anyway, results flagged in the report
#
# Tunables: BENCH_CPU (pin core, default 2), MIN_RUNS (default 30),
#           WARMUP (default 10), CONF_RUNS (configure runs, default 5),
#           SKIP_CONFIGURE=1 to skip the autoconf dimension.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BENCH="$ROOT/bench"
ART="$BENCH/.artifacts/perf"
GEN="$BENCH/workloads/gen"
CPU="${BENCH_CPU:-2}"
MIN_RUNS="${MIN_RUNS:-30}"
WARMUP="${WARMUP:-10}"
CONF_RUNS="${CONF_RUNS:-5}"

BASH_BIN=/bin/bash
DASH_BIN=/usr/bin/dash
HELLISH="$BENCH/.bin/hellish"

# ---- environment gate -----------------------------------------------------
GOV="$(cat /sys/devices/system/cpu/cpu"$CPU"/cpufreq/scaling_governor 2>/dev/null || echo unknown)"
if [ "$GOV" != performance ]; then
    echo "!! CPU $CPU governor is '$GOV', not 'performance'." >&2
    echo "!! Fix:  sudo cpupower frequency-set -g performance" >&2
    if [ "${BENCH_LAX:-0}" != 1 ]; then
        echo "!! Refusing to produce numbers on a throttling CPU." >&2
        echo "!! (export BENCH_LAX=1 to run anyway; the report will flag it)" >&2
        exit 1
    fi
    echo "!! BENCH_LAX=1: continuing; results will be flagged." >&2
fi

# ---- setup ----------------------------------------------------------------
cd "$ROOT"
/bin/bash bench/lib/fetch_suites.sh
make --no-print-directory OPT=1 >/dev/null
mkdir -p bench/.bin && cp build/bin/hellish "$HELLISH"
/bin/bash bench/lib/gen_workloads.sh
mkdir -p "$ART"

HYPERFINE="$(command -v hyperfine || echo "$BENCH/.bin/hyperfine")"
T="taskset -c $CPU"

# Every measured command is exec'd directly by hyperfine (-N: no shell).
# The only wrappers are taskset (identical for every shell) and, for the
# configure dimension, env (to set CONFIG_SHELL) — neither is a shell.
bench_script() {
    name="$1"; script="$2"; shift 2
    echo "== $name" >&2
    "$HYPERFINE" -N --warmup "$WARMUP" --min-runs "$MIN_RUNS" \
        --export-json "$ART/$name.json" "$@" \
        -n hellish "$T $HELLISH --posix $script" \
        -n bash    "$T $BASH_BIN --norc --posix $script" \
        -n dash    "$T $DASH_BIN $script"
}

# ---- a) startup -----------------------------------------------------------
echo "== startup" >&2
"$HYPERFINE" -N --warmup "$WARMUP" --min-runs "$MIN_RUNS" \
    --export-json "$ART/startup.json" \
    -n hellish "$T $HELLISH --posix -c true" \
    -n bash    "$T $BASH_BIN --norc --posix -c true" \
    -n dash    "$T $DASH_BIN -c true"

# ---- b) parser throughput -------------------------------------------------
bench_script parse50k "$GEN/parse50k.sh"

# ---- c) loop / builtin throughput ----------------------------------------
bench_script loop_arith  "$GEN/loop_arith.sh"
bench_script loop_concat "$GEN/loop_concat.sh"
bench_script loop_colon  "$GEN/loop_colon.sh"
bench_script loop_read   "$GEN/loop_read.sh"

# ---- d) fork workloads ----------------------------------------------------
bench_script fork_cmdsub     "$GEN/fork_cmdsub.sh"
bench_script fork_cmdsub_ext "$GEN/fork_cmdsub_ext.sh"
bench_script fork_pipeline   "$GEN/fork_pipeline.sh"

# ---- e) real workload: autoconf configure --------------------------------
if [ "${SKIP_CONFIGURE:-0}" != 1 ]; then
    HELLO="$BENCH/workloads/hello-2.12.1"
    BUILD="$BENCH/.artifacts/configure-build"
    rm -rf "$BUILD" && mkdir -p "$BUILD" && cd "$BUILD"
    echo "== configure (CONFIG_SHELL, $CONF_RUNS runs/shell — slow)" >&2
    "$HYPERFINE" -N --warmup 1 --min-runs "$CONF_RUNS" \
        --export-json "$ART/configure.json" \
        --prepare "find $BUILD -mindepth 1 -delete" \
        -n hellish "$T env CONFIG_SHELL=$HELLISH $HELLISH --posix $HELLO/configure --quiet" \
        -n bash    "$T env CONFIG_SHELL=$BASH_BIN $BASH_BIN --norc --posix $HELLO/configure --quiet" \
        -n dash    "$T env CONFIG_SHELL=$DASH_BIN $DASH_BIN $HELLO/configure --quiet"
    cd "$ROOT"
fi

# ---- report ---------------------------------------------------------------
GOVERNOR="$GOV" python3 bench/lib/report_perf.py
echo "report: bench/results.md" >&2

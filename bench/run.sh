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
# Governor policy: warn-and-continue by default so `make perf` works on a
# machine without sudo (the report flags every result's reliability via CV
# and a prominent banner).  Set BENCH_STRICT=1 to demand the performance
# governor and refuse otherwise -- use that when you DO have sudo and want
# publication-grade numbers.
GOV="$(cat /sys/devices/system/cpu/cpu"$CPU"/cpufreq/scaling_governor 2>/dev/null || echo unknown)"
if [ "$GOV" != performance ]; then
    echo "!! CPU $CPU governor is '$GOV', not 'performance'." >&2
    echo "!! For publication-grade numbers: sudo cpupower frequency-set -g performance" >&2
    if [ "${BENCH_STRICT:-0}" = 1 ]; then
        echo "!! BENCH_STRICT=1: refusing to produce numbers on a throttling CPU." >&2
        exit 1
    fi
    echo "!! Continuing anyway; results.md will be flagged as governor-limited." >&2
    echo "!! (set BENCH_STRICT=1 to refuse instead.)" >&2
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
# Per-command timeout safety net (a real binary, not a shell). Configure runs
# legitimately take ~10s, so its guard is larger.
TG="timeout ${CMD_TIMEOUT:-45}"
TGC="timeout ${CONF_TIMEOUT:-120}"

# Every measured command is exec'd directly by hyperfine (-N: no shell).
# Wrappers are taskset (identical for every shell), env (configure dimension
# only, to set CONFIG_SHELL) and timeout — none of which is a shell.  The
# `timeout` prefix is a safety net: a mis-sized or accidentally-hanging
# command dies at CMD_TIMEOUT instead of wedging the whole suite.  With the
# workloads right-sized it never fires.
#
# bench_script <name> <script> [runs] [warmup]
# Heavy dimensions (a single run near/over a second) take fewer repetitions
# so the whole suite stays a few minutes; light ones keep the full 30.
bench_script() {
    name="$1"; script="$2"; runs="${3:-$MIN_RUNS}"; warm="${4:-$WARMUP}"
    echo "== $name (${runs} runs)" >&2
    "$HYPERFINE" -N --warmup "$warm" --min-runs "$runs" \
        --export-json "$ART/$name.json" \
        -n hellish "$TG $T $HELLISH --posix $script" \
        -n bash    "$TG $T $BASH_BIN --norc --posix $script" \
        -n dash    "$TG $T $DASH_BIN $script"
}

# ---- a) startup -----------------------------------------------------------
echo "== startup" >&2
"$HYPERFINE" -N --warmup "$WARMUP" --min-runs "$MIN_RUNS" \
    --export-json "$ART/startup.json" \
    -n hellish "$TG $T $HELLISH --posix -c true" \
    -n bash    "$TG $T $BASH_BIN --norc --posix -c true" \
    -n dash    "$TG $T $DASH_BIN -c true"

# ---- b) parser throughput -------------------------------------------------
bench_script parse50k "$GEN/parse50k.sh" 15 3

# ---- c) loop / builtin throughput ----------------------------------------
bench_script loop_arith  "$GEN/loop_arith.sh"
bench_script loop_concat "$GEN/loop_concat.sh" 20 5
bench_script loop_colon  "$GEN/loop_colon.sh"
bench_script loop_read   "$GEN/loop_read.sh" 15 3

# ---- d) fork workloads ----------------------------------------------------
bench_script fork_cmdsub     "$GEN/fork_cmdsub.sh"
bench_script fork_cmdsub_ext "$GEN/fork_cmdsub_ext.sh" 20 5
bench_script fork_pipeline   "$GEN/fork_pipeline.sh" 15 3

# ---- e) real workload: autoconf configure --------------------------------
# Only shells that actually COMPLETE configure (produce config.status) are
# timed -- benchmarking a fast failure against a real run would be a lie.
# A non-completing shell is recorded in configure-skipped.txt with its exit
# status, which report_perf.py surfaces as an honest N/A row.
if [ "${SKIP_CONFIGURE:-0}" != 1 ]; then
    HELLO="$BENCH/workloads/hello-2.12.1"
    BUILD="$BENCH/.artifacts/configure-build"
    rm -f "$ART/configure-skipped.txt"
    # (label, binary, flags) for the three shells.
    conf_labels=(hellish bash dash)
    conf_bins=("$HELLISH" "$BASH_BIN" "$DASH_BIN")
    conf_flags=("--posix" "--norc --posix" "")
    hf=()
    for i in "${!conf_labels[@]}"; do
        lbl="${conf_labels[$i]}"; bin="${conf_bins[$i]}"; fl="${conf_flags[$i]}"
        rm -rf "$BUILD" && mkdir -p "$BUILD" && cd "$BUILD"
        $TGC env CONFIG_SHELL="$bin" $bin $fl "$HELLO/configure" --quiet \
            >/dev/null 2>&1
        cd "$ROOT"
        if [ -f "$BUILD/config.status" ]; then
            hf+=(-n "$lbl" "$TGC $T env CONFIG_SHELL=$bin $bin $fl $HELLO/configure --quiet")
        else
            echo "$lbl (does not complete configure: no config.status)" \
                >> "$ART/configure-skipped.txt"
            echo "!! $lbl does not complete configure -- excluded from timing" >&2
        fi
    done
    rm -rf "$BUILD" && mkdir -p "$BUILD" && cd "$BUILD"
    echo "== configure (CONFIG_SHELL, $CONF_RUNS runs/shell — slow)" >&2
    if [ "${#hf[@]}" -gt 0 ]; then
        "$HYPERFINE" -N --warmup 1 --min-runs "$CONF_RUNS" \
            --export-json "$ART/configure.json" \
            --prepare "find $BUILD -mindepth 1 -delete" "${hf[@]}" || true
    fi
    cd "$ROOT"
fi

# ---- report ---------------------------------------------------------------
GOVERNOR="$GOV" python3 bench/lib/report_perf.py
echo "report: bench/results.md" >&2

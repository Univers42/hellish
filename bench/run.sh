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

# ---- concurrency guard ----------------------------------------------------
# Two perf runs sharing bench/.bin/hellish corrupt each other's timing and
# collide on the binary copy ("Text file busy").  A dir-based lock keeps
# make perf single-flight; a stale lock (>1h, or empty) is reclaimed.
LOCK="$BENCH/.artifacts/perf.lock"
mkdir -p "$ART"
if ! mkdir "$LOCK" 2>/dev/null; then
    echo "!! another 'make perf' looks to be running ($LOCK)." >&2
    echo "!! if that is stale, remove it:  rm -rf $LOCK" >&2
    exit 1
fi
trap 'rmdir "$LOCK" 2>/dev/null || true' EXIT

# ---- setup ----------------------------------------------------------------
cd "$ROOT"
/bin/bash bench/lib/fetch_suites.sh
make --no-print-directory OPT=1 >/dev/null
# Update the pinned binary ATOMICALLY: cp to a temp then rename.  A plain
# `cp` over a binary that a previous/concurrent run is still executing fails
# with "Text file busy"; rename() swaps the directory entry (new inode) and
# leaves any running copy on its old inode, so it always succeeds.
mkdir -p bench/.bin
cp build/bin/hellish "$HELLISH.tmp.$$"
mv -f "$HELLISH.tmp.$$" "$HELLISH"
/bin/bash bench/lib/gen_workloads.sh

HYPERFINE="$(command -v hyperfine || echo "$BENCH/.bin/hyperfine")"
T="taskset -c $CPU"
# Pick a timeout(1) that reaps its child on a SIGCHLD instead of polling.
# This matters more than it looks: uutils coreutils' timeout (the Rust rewrite
# that ships as /usr/bin/timeout on some distros) waits on a 100ms grid, so it
# adds up to ~100ms to EVERY measured command and rounds the result up to the
# next 100ms bucket -- `dash -c true` reads 103ms instead of 0.5ms. That floor
# silently dominates every sub-second dimension and turns the ratios into
# rounding artifacts. GNU coreutils' timeout costs ~1ms, so we hunt for it
# (often installed alongside as `gnutimeout`) and only fall back to whatever
# `timeout` is on PATH -- warning loudly, because those numbers cannot be
# trusted below a second.
pick_timeout() {
    for cand in gnutimeout gtimeout timeout; do
        bin="$(command -v "$cand" 2>/dev/null)" || continue
        if "$bin" --version 2>/dev/null | head -1 | grep -qi 'GNU coreutils'; then
            printf '%s' "$bin"
            return 0
        fi
    done
    return 1
}
if TIMEOUT_BIN="$(pick_timeout)"; then
    :
else
    TIMEOUT_BIN="$(command -v timeout || true)"
    echo "!! no GNU timeout found; using '${TIMEOUT_BIN:-none}'." >&2
    echo "!! non-GNU timeout implementations can poll on a 100ms grid and" >&2
    echo "!! inflate every sub-second measurement. Install GNU coreutils." >&2
fi
TG="$TIMEOUT_BIN ${CMD_TIMEOUT:-45}"
TGC="$TIMEOUT_BIN ${CONF_TIMEOUT:-120}"

# Print a HELLISH-centric one-line verdict for a finished dimension, read from
# its JSON.  hyperfine's own summary picks whichever shell was fastest as the
# reference ("dash ran N x faster than hellish"), which reads differently every
# dimension; here every ratio is other/hellish, so > 1 always means hellish is
# faster.  We suppress hyperfine's summary (stdout) and show this instead.
perf_line() {
	python3 - "$ART/$1.json" "$1" 2>/dev/null <<'PY'
import json, sys
try:
    d = json.load(open(sys.argv[1]))
except Exception:
    sys.exit(0)
m = {r["command"]: r["median"] for r in d["results"]}
h = m.get("hellish")
def fmt(t):
    if not t:
        return "n/a"
    return f"{t*1000:.0f}ms" if t < 1 else f"{t:.2f}s"
def cell(name):
    o = m.get(name)
    if o is None or not h:
        return f"vs {name} n/a"
    r = o / h
    tag = "faster" if r >= 1.05 else ("SLOWER" if r <= 0.95 else "~tie")
    return f"vs {name} {r:.2f}x ({tag})"
if not h:
    print(f"  ▶ {sys.argv[2]}: hellish did not run (excluded)")
    sys.exit(0)
fastest = min(m, key=m.get)
tail = " -> hellish fastest" if fastest == "hellish" else f" -> fastest: {fastest}"
print(f"  ▶ {sys.argv[2]}: hellish {fmt(h)} | {cell('bash')} | {cell('dash')}{tail}")
PY
}

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
    # -i tolerates a non-zero exit from one shell; `|| true` keeps `set -e`
    # from killing the run (and the final report) if a dimension errors.
    # Progress bars stay on stderr; hyperfine's own (fastest-relative) result
    # blocks go to /dev/null, replaced by the hellish-centric perf_line below.
    "$HYPERFINE" -N -i --warmup "$warm" --min-runs "$runs" \
        --export-json "$ART/$name.json" \
        -n hellish "$TG $T $HELLISH --posix $script" \
        -n bash    "$TG $T $BASH_BIN --norc --posix $script" \
        -n dash    "$TG $T $DASH_BIN $script" >/dev/null 2>&1 || true
    perf_line "$name"
}

# ---- a) startup -----------------------------------------------------------
echo "== startup" >&2
"$HYPERFINE" -N -i --warmup "$WARMUP" --min-runs "$MIN_RUNS" \
    --export-json "$ART/startup.json" \
    -n hellish "$TG $T $HELLISH --posix -c true" \
    -n bash    "$TG $T $BASH_BIN --norc --posix -c true" \
    -n dash    "$TG $T $DASH_BIN -c true" >/dev/null 2>&1 || true
perf_line startup

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
        # `|| true`: a shell that fails configure exits non-zero, which under
        # `set -e` would kill the whole run before the report is written.
        $TGC env CONFIG_SHELL="$bin" $bin $fl "$HELLO/configure" --quiet \
            >/dev/null 2>&1 || true
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
        "$HYPERFINE" -N -i --warmup 1 --min-runs "$CONF_RUNS" \
            --export-json "$ART/configure.json" \
            --prepare "find $BUILD -mindepth 1 -delete" "${hf[@]}" \
            >/dev/null 2>&1 || true
        perf_line configure
    fi
    cd "$ROOT"
fi

# ---- report ---------------------------------------------------------------
GOVERNOR="$GOV" python3 bench/lib/report_perf.py
echo "report: bench/results.md" >&2

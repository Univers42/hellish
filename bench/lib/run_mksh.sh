#!/bin/bash
# Run mksh's own regression suite (check.t via check.pl) against each shell.
# Everything mksh-specific fails for ALL foreign shells alike — the value is
# the relative standing of hellish vs bash --posix vs dash on an
# independent, third-party harness, not the absolute pass count.
set -u

BENCH_DIR="$(cd "$(dirname "$0")/.." && pwd)"
MKSH="$BENCH_DIR/suites/mksh"
ART="$BENCH_DIR/.artifacts"
HELLISH="${HELLISH:-$BENCH_DIR/.bin/hellish}"

mkdir -p "$ART"
TMP_ROOT="$(mktemp -d /tmp/hellish-mksh-XXXXXX)"
trap 'rm -rf "$TMP_ROOT"' EXIT

run_one() {
    label="$1"; shift
    mkdir -p "$TMP_ROOT/$label"
    # -P: the -p argument carries flags; -t 10: per-test timeout.
    (cd "$MKSH" && perl check.pl -s check.t -P -p "$*" -t 10 \
        -T "$TMP_ROOT/$label") > "$ART/mksh-$label.txt" 2>&1
    echo "mksh/$label: $(grep -E '^Total (passed|failed)' "$ART/mksh-$label.txt" | tr '\n' ' ')" >&2
}

run_one bash    /bin/bash --posix
run_one dash    /usr/bin/dash
run_one hellish "$HELLISH" --posix

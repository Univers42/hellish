#!/bin/bash
# Run the Oils spec-test corpus (files whose `## compare_shells:` includes
# dash — i.e. the POSIX-comparison set) against bash --posix, dash, and
# hellish.  One TSV per spec file lands in bench/.artifacts/oils/, plus the
# human-readable ANSI table for debugging.
#
# The sh_spec.py runner exits non-zero whenever there are failures at all;
# we ignore its exit status and judge from the TSVs.  A file whose runner
# crashes (no TSV produced) is recorded in oils-skipped.txt with its error.
set -u

BENCH_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OILS="$BENCH_DIR/suites/oils"
ART="$BENCH_DIR/.artifacts/oils"
HELLISH="${HELLISH:-$BENCH_DIR/.bin/hellish}"
BASH_BIN="${BASH_BIN:-/bin/bash}"
DASH_BIN="${DASH_BIN:-/usr/bin/dash}"
CASE_TIMEOUT="${CASE_TIMEOUT:-5}"
FILE_TIMEOUT="${FILE_TIMEOUT:-600}"

mkdir -p "$ART"
# RESUME=1 keeps existing per-file TSVs and only runs the missing ones —
# for continuing an interrupted sweep.  Default is a clean rerun.
if [ "${RESUME:-0}" != 1 ]; then
    rm -f "$ART"/*.tsv "$ART"/*.txt "$ART/oils-skipped.txt"
fi

TMP_ROOT="$(mktemp -d /tmp/hellish-oils-XXXXXX)"
trap 'rm -rf "$TMP_ROOT"' EXIT

# The POSIX-comparison corpus: every spec file that names dash as a
# comparison shell.  Files that break the python3-ported runner are listed
# in oils-exclude.txt (one name + reason per line) and skipped up front.
FILES=$(grep -l '^## compare_shells:.*dash' "$OILS"/spec/*.test.sh | sort)
EXCLUDE="$BENCH_DIR/lib/oils-exclude.txt"

total=0
for f in $FILES; do
    name=$(basename "$f" .test.sh)
    if [ -f "$EXCLUDE" ] && grep -q "^$name\b" "$EXCLUDE"; then
        echo "$name (excluded: $(grep "^$name\b" "$EXCLUDE" | cut -d' ' -f2-))" \
            >> "$ART/oils-skipped.txt"
        continue
    fi
    if [ "${RESUME:-0}" = 1 ] && [ -s "$ART/$name.tsv" ]; then
        continue
    fi
    total=$((total + 1))
    mkdir -p "$TMP_ROOT/$name"
    timeout "$FILE_TIMEOUT" env PYTHONPATH="$OILS" python3 "$OILS/test/sh_spec.py" \
        --timeout "$CASE_TIMEOUT" --posix \
        --tmp-env "$TMP_ROOT/$name" \
        --path-env "$PATH:$OILS/spec/bin" \
        --tsv-output "$ART/$name.tsv" \
        "$f" "$BASH_BIN" "$DASH_BIN" "$HELLISH" \
        > "$ART/$name.txt" 2> "$ART/$name.err"
    if [ ! -s "$ART/$name.tsv" ]; then
        echo "$name (runner error: $(tail -1 "$ART/$name.err" 2>/dev/null | head -c 120))" \
            >> "$ART/oils-skipped.txt"
        rm -f "$ART/$name.tsv"
    fi
    printf '.' >&2
done
printf '\n%d spec files attempted\n' "$total" >&2

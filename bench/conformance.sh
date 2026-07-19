#!/bin/bash
# Conformance driver: rebuild the OPT binary, pin it, run both third-party
# suites, aggregate into bench/conformance.md, and enforce the regression
# gate (hellish's pass counts must never drop vs bench/baseline/).
#
#   UPDATE_BASELINE=1 bench/conformance.sh   # accept current counts as baseline
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

make --no-print-directory OPT=1 >/dev/null
mkdir -p bench/.bin bench/.artifacts
cp build/bin/hellish bench/.bin/hellish

echo "== Oils spec tests (this takes a few minutes)" >&2
/bin/bash bench/lib/run_oils.sh

echo "== mksh check.t" >&2
/bin/bash bench/lib/run_mksh.sh

echo "== aggregate + gate" >&2
python3 bench/lib/aggregate_conformance.py
echo "report: bench/conformance.md" >&2

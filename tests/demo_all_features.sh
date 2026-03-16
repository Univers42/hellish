#!/bin/bash
# ============================================================
#  MASTER DEMO: Run all 42sh feature demos
# ============================================================
DIR="$(cd "$(dirname "$0")" && pwd)"
SHELL_BIN="${1:-/home/dylan/sh42/build/bin/hellish}"

TOTAL_PASS=0
TOTAL_FAIL=0

run_demo() {
  name="$1"
  script="$2"
  output=$(/bin/bash "$DIR/$script" "$SHELL_BIN" 2>&1)
  echo "$output"
  p=$(echo "$output" | grep "RESULTS:" | grep -oP '\d+ passed' | grep -oP '\d+')
  f=$(echo "$output" | grep "RESULTS:" | grep -oP '\d+ failed' | grep -oP '\d+')
  TOTAL_PASS=$((TOTAL_PASS + ${p:-0}))
  TOTAL_FAIL=$((TOTAL_FAIL + ${f:-0}))
}

echo ""
echo "╔══════════════════════════════════════════════╗"
echo "║    42sh NEW FEATURES - COMPREHENSIVE DEMO    ║"
echo "╚══════════════════════════════════════════════╝"
echo ""
echo "Shell binary: $SHELL_BIN"
echo "Date: $(date)"
echo ""

run_demo "test/[ builtin"     "demo_test_builtin.sh"
run_demo "alias/unalias"      "demo_alias.sh"
run_demo "hash builtin"       "demo_hash.sh"
run_demo "type integration"   "demo_type.sh"
run_demo "set -o vi/emacs"    "demo_set_o.sh"
run_demo "jobs/fg/bg"         "demo_jobs.sh"
run_demo "fc builtin"         "demo_fc.sh"
run_demo "history expansion"  "demo_history_expand.sh"

echo ""
echo "╔══════════════════════════════════════════════╗"
echo "║            GRAND TOTAL RESULTS               ║"
echo "╠══════════════════════════════════════════════╣"
printf "║  \033[32m%3d passed\033[0m                                  ║\n" "$TOTAL_PASS"
printf "║  \033[31m%3d failed\033[0m                                  ║\n" "$TOTAL_FAIL"
TOTAL=$((TOTAL_PASS + TOTAL_FAIL))
if [ "$TOTAL_FAIL" -eq 0 ]; then
  printf "║  \033[1;32m★  ALL %d TESTS PASSED  ★\033[0m                   ║\n" "$TOTAL"
else
  printf "║  \033[1;31m✗  %d / %d TESTS FAILED\033[0m                    ║\n" "$TOTAL_FAIL" "$TOTAL"
fi
echo "╚══════════════════════════════════════════════╝"
echo ""

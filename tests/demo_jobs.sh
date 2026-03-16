#!/bin/bash
# ============================================================
#  DEMO: jobs / fg / bg builtins
#  NOTE: Full job control requires an interactive terminal.
#        This demo tests the non-interactive aspects (no crash,
#        correct exit codes, proper error messages).
# ============================================================
SHELL_BIN="${1:-/home/dylan/sh42/build/bin/hellish}"
PASS=0; FAIL=0

check() {
  label="$1"; expected="$2"; got="$3"
  if [ "$got" = "$expected" ]; then
    printf "  \033[32m✓\033[0m %-50s\n" "$label"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-50s\n    expected: [%s]\n    got:      [%s]\n" "$label" "$expected" "$got"
    FAIL=$((FAIL+1))
  fi
}

check_contains() {
  label="$1"; pattern="$2"; got="$3"
  if echo "$got" | grep -q "$pattern"; then
    printf "  \033[32m✓\033[0m %-50s\n" "$label"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-50s\n    expected to contain: [%s]\n    got: [%s]\n" "$label" "$pattern" "$got"
    FAIL=$((FAIL+1))
  fi
}

echo ""
echo "=========================================="
echo "  DEMO: jobs / fg / bg"
echo "=========================================="
echo "  (Note: full job control requires interactive TTY)"

# --- jobs with no jobs ---
echo ""
echo "--- jobs (empty) ---"

got=$(printf 'jobs\n' | "$SHELL_BIN" 2>/dev/null)
check "jobs (no jobs) → empty output" "" "$got"

got=$(printf 'jobs\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "jobs exit code → 0" "0" "$got"

# --- jobs -l (long format, no jobs) ---
got=$(printf 'jobs -l\n' | "$SHELL_BIN" 2>/dev/null)
check "jobs -l (no jobs) → empty" "" "$got"

# --- jobs -p (pid only, no jobs) ---
got=$(printf 'jobs -p\n' | "$SHELL_BIN" 2>/dev/null)
check "jobs -p (no jobs) → empty" "" "$got"

# --- fg with no jobs ---
echo ""
echo "--- fg/bg with no jobs ---"

got=$(printf 'fg 2>&1\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "fg (no jobs) → exit 1" "1" "$got"

got=$(printf 'bg 2>&1\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "bg (no jobs) → exit 1" "1" "$got"

# --- fg error message ---
got=$(printf 'fg 2>&1\n' | "$SHELL_BIN" 2>&1 | head -1)
check_contains "fg with no jobs → error message" "no.*job\|no current job\|no such job" "$got"

got=$(printf 'bg 2>&1\n' | "$SHELL_BIN" 2>&1 | head -1)
check_contains "bg with no jobs → error message" "no.*job\|no current job\|no such job" "$got"

# --- type checks ---
echo ""
echo "--- type recognition ---"

got=$(printf 'type jobs\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type jobs → builtin" "builtin" "$got"

got=$(printf 'type fg\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type fg → builtin" "builtin" "$got"

got=$(printf 'type bg\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type bg → builtin" "builtin" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

echo "==========================================="
echo "  INTERACTIVE JOB CONTROL DEMO"
echo "==========================================="
echo "  To test interactively, run hellish and try:"
echo ""
echo "    sleep 100 &         # start background job"
echo "    jobs                # list jobs"
echo "    jobs -l             # list with PIDs"
echo "    fg %1               # bring to foreground"
echo "    <Ctrl-Z>            # suspend it"
echo "    bg %1               # resume in background"
echo "    jobs                # verify 'Running'"
echo ""

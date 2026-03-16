#!/bin/bash
# ============================================================
#  DEMO: History Expansion (!!, !word, !N, !-N)
#  NOTE: History expansion requires interactive readline.
#        In non-interactive (pipe) mode, history is not recorded
#        so expansion won't fire. This demo verifies:
#        1) The code doesn't crash
#        2) The expansion logic is sound (via code audit)
#        We also test edge cases that should NOT expand.
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

echo ""
echo "=========================================="
echo "  DEMO: History Expansion"
echo "=========================================="
echo "  (History expansion requires interactive readline)"

# --- No crash with ! in non-interactive mode ---
echo ""
echo "--- Safety: no crash with bang patterns ---"

got=$(printf 'echo "hello"\necho "!!"\n' | "$SHELL_BIN" 2>/dev/null)
last_line=$(echo "$got" | tail -1)
# In non-interactive, !! is literal (history not active)
check "echo '!!' in non-interactive → no crash" "!!" "$last_line"

got=$(printf 'echo test\necho "!echo"\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "echo '!echo' in non-interactive → no crash" "!echo" "$got"

got=$(printf 'echo "!42"\n' | "$SHELL_BIN" 2>/dev/null)
check "echo '!42' in non-interactive → no crash" "!42" "$got"

got=$(printf 'echo "!-1"\n' | "$SHELL_BIN" 2>/dev/null)
check "echo '!-1' in non-interactive → no crash" "!-1" "$got"

# --- ! inside single quotes should be literal ---
echo ""
echo "--- Single quotes protect ! ---"

got=$(printf "echo '!!'\n" | "$SHELL_BIN" 2>/dev/null)
check "echo '!!' (single quoted) → literal" "!!" "$got"

got=$(printf "echo '!hello'\n" | "$SHELL_BIN" 2>/dev/null)
check "echo '!hello' (single quoted) → literal" "!hello" "$got"

# --- ! at end of line or with space ---
echo ""
echo "--- ! edge cases ---"

got=$(printf 'echo hello!\n' | "$SHELL_BIN" 2>/dev/null)
check "echo hello! (trailing !) → no expand" "hello!" "$got"

got=$(printf 'echo "! space"\n' | "$SHELL_BIN" 2>/dev/null)
check "echo '! space' (! + space) → literal" "! space" "$got"

# --- != should not trigger expansion ---
got=$(printf 'test "a" != "b"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test != operator → not confused with !-expansion" "0" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

echo "==========================================="
echo "  INTERACTIVE HISTORY EXPANSION DEMO"
echo "==========================================="
echo "  To test interactively, run hellish and try:"
echo ""
echo "    echo hello world"
echo "    !!                  # repeats: echo hello world"
echo "    echo first"
echo "    echo second"
echo "    !echo               # repeats: echo second"
echo "    !1                  # repeats: 1st command"
echo "    !-2                 # repeats: 2nd-to-last"
echo "    echo '!!' not expanded in single quotes"
echo ""

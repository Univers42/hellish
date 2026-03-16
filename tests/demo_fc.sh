#!/bin/bash
# ============================================================
#  DEMO: fc builtin (fix command / history)
#  NOTE: fc relies on readline history, which requires
#        interactive mode. This tests non-interactive behavior
#        and error handling.
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
  if echo "$got" | grep -qi "$pattern"; then
    printf "  \033[32m✓\033[0m %-50s\n" "$label"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-50s\n    expected to contain: [%s]\n    got: [%s]\n" "$label" "$pattern" "$got"
    FAIL=$((FAIL+1))
  fi
}

echo ""
echo "=========================================="
echo "  DEMO: fc builtin"
echo "=========================================="
echo "  (Note: fc requires interactive readline history)"

# --- fc is recognized as builtin ---
echo ""
echo "--- fc type check ---"

got=$(printf 'type fc\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type fc → builtin" "builtin" "$got"

# --- fc -l with no history (non-interactive) ---
echo ""
echo "--- fc -l (non-interactive, no history) ---"

got=$(printf 'fc -l\n' | "$SHELL_BIN" 2>&1)
check_contains "fc -l with no history → error message" "no.*command.*history\|no.*history\|history" "$got"

got=$(printf 'fc -l\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "fc -l with no history → exit 1" "1" "$got"

# --- fc with no history ---
got=$(printf 'fc\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "fc with no history → exit 1" "1" "$got"

# --- fc -l with range (no history) ---
got=$(printf 'fc -l 1 5\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "fc -l 1 5 (no history) → exit 1" "1" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

echo "==========================================="
echo "  INTERACTIVE fc DEMO"
echo "==========================================="
echo "  To test fc interactively, run hellish and try:"
echo ""
echo "    echo hello"
echo "    echo world"
echo "    echo 42"
echo "    fc -l              # list last 16 commands"
echo "    fc -l 1 3          # list commands 1-3"
echo "    fc -l -3           # list last 3 commands"
echo "    fc -e vi           # edit last cmd in vi"
echo "    fc                 # edit last cmd in \$FCEDIT"
echo ""

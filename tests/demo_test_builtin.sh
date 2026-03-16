#!/bin/bash
# ============================================================
#  DEMO: test / [ builtin
# ============================================================
SHELL_BIN="${1:-/home/dylan/sh42/build/bin/hellish}"
PASS=0; FAIL=0

check() {
  label="$1"; expected="$2"; got="$3"
  if [ "$got" = "$expected" ]; then
    printf "  \033[32m✓\033[0m %-45s (expected %s, got %s)\n" "$label" "$expected" "$got"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-45s (expected %s, got %s)\n" "$label" "$expected" "$got"
    FAIL=$((FAIL+1))
  fi
}

echo ""
echo "=========================================="
echo "  DEMO: test / [ builtin"
echo "=========================================="

# --- File tests ---
echo ""
echo "--- File operators ---"

got=$(printf 'test -d /tmp; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -d /tmp (directory exists)" "0" "$got"

got=$(printf 'test -f /etc/passwd; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -f /etc/passwd (file exists)" "0" "$got"

got=$(printf 'test -f /nonexistent_file; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -f /nonexistent (does not exist)" "1" "$got"

got=$(printf 'test -e /dev/null; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -e /dev/null (exists)" "0" "$got"

got=$(printf 'test -r /etc/passwd; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -r /etc/passwd (readable)" "0" "$got"

got=$(printf 'test -w /etc/shadow; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -w /etc/shadow (not writable)" "1" "$got"

got=$(printf 'test -x /bin/ls; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -x /bin/ls (executable)" "0" "$got"

got=$(printf 'test -s /etc/passwd; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -s /etc/passwd (non-empty)" "0" "$got"

got=$(printf 'test -L /dev/stdin; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test -L /dev/stdin (symlink)" "0" "$got"

# --- String tests ---
echo ""
echo "--- String operators ---"

got=$(printf 'test -z ""; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test -z "" (empty string)' "0" "$got"

got=$(printf 'test -z "hello"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test -z "hello" (non-empty)' "1" "$got"

got=$(printf 'test -n "hello"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test -n "hello" (non-empty)' "0" "$got"

got=$(printf 'test -n ""; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test -n "" (empty)' "1" "$got"

got=$(printf 'test "abc" = "abc"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test "abc" = "abc" (equal)' "0" "$got"

got=$(printf 'test "abc" = "xyz"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test "abc" = "xyz" (not equal)' "1" "$got"

got=$(printf 'test "abc" != "xyz"; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check 'test "abc" != "xyz" (not equal)' "0" "$got"

# --- Integer tests ---
echo ""
echo "--- Integer operators ---"

got=$(printf 'test 42 -eq 42; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 42 -eq 42" "0" "$got"

got=$(printf 'test 42 -ne 99; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 42 -ne 99" "0" "$got"

got=$(printf 'test 10 -gt 5; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 10 -gt 5" "0" "$got"

got=$(printf 'test 5 -lt 10; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 5 -lt 10" "0" "$got"

got=$(printf 'test 5 -ge 5; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 5 -ge 5" "0" "$got"

got=$(printf 'test 3 -le 5; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test 3 -le 5" "0" "$got"

# --- [ syntax ---
echo ""
echo "--- [ ] bracket syntax ---"

got=$(printf '[ -d /tmp ]; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "[ -d /tmp ]" "0" "$got"

got=$(printf '[ 5 -gt 3 ]; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "[ 5 -gt 3 ]" "0" "$got"

got=$(printf '[ -z "" ]; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check '[ -z "" ]' "0" "$got"

got=$(printf '[ "a" = "a" ]; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check '[ "a" = "a" ]' "0" "$got"

# --- Negation ---
echo ""
echo "--- Negation (!) ---"

got=$(printf 'test ! -f /nonexistent; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test ! -f /nonexistent (negated true)" "0" "$got"

got=$(printf 'test ! -d /tmp; echo $?\n' | "$SHELL_BIN" 2>/dev/null)
check "test ! -d /tmp (negated false)" "1" "$got"

# --- if/test integration ---
echo ""
echo "--- if/test integration ---"

got=$(printf 'if test -d /tmp; then echo yes; else echo no; fi\n' | "$SHELL_BIN" 2>/dev/null)
check "if test -d /tmp → yes" "yes" "$got"

got=$(printf 'if [ 5 -gt 3 ]; then echo bigger; else echo smaller; fi\n' | "$SHELL_BIN" 2>/dev/null)
check "if [ 5 -gt 3 ] → bigger" "bigger" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

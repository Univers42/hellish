#!/bin/bash
# ============================================================
#  DEMO: hash builtin (command hash table)
# ============================================================
SHELL_BIN="${1:-/home/dylan/sh42/build/bin/hellish}"
PASS=0; FAIL=0

check() {
  label="$1"; expected="$2"; got="$3"
  if [ "$got" = "$expected" ]; then
    printf "  \033[32m✓\033[0m %-45s\n" "$label"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-45s\n    expected: [%s]\n    got:      [%s]\n" "$label" "$expected" "$got"
    FAIL=$((FAIL+1))
  fi
}

check_contains() {
  label="$1"; pattern="$2"; got="$3"
  if echo "$got" | grep -q "$pattern"; then
    printf "  \033[32m✓\033[0m %-45s\n" "$label"
    PASS=$((PASS+1))
  else
    printf "  \033[31m✗\033[0m %-45s\n    expected to contain: [%s]\n    got: [%s]\n" "$label" "$pattern" "$got"
    FAIL=$((FAIL+1))
  fi
}

echo ""
echo "=========================================="
echo "  DEMO: hash builtin"
echo "=========================================="

# --- Empty hash table ---
echo ""
echo "--- Empty hash table ---"

got=$(printf 'hash\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "hash (empty) → header only" "hits" "$got"

# Line count: should only be the header
lines=$(printf 'hash\n' | "$SHELL_BIN" 2>/dev/null | wc -l)
check "hash (empty) → 1 line (header)" "1" "$lines"

# --- Manual hash add ---
echo ""
echo "--- hash <name> (manual add from PATH) ---"

got=$(printf 'hash ls\nhash\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "hash ls → adds /usr/bin/ls" "/usr/bin/ls" "$got"

got=$(printf 'hash cat\nhash grep\nhash\n' | "$SHELL_BIN" 2>/dev/null | grep -c "/usr")
check "hash cat + grep → 2 entries" "2" "$got"

# --- hash -r (clear) ---
echo ""
echo "--- hash -r (clear all) ---"

lines=$(printf 'hash ls\nhash cat\nhash -r\nhash\n' | "$SHELL_BIN" 2>/dev/null | wc -l)
check "hash -r clears table → 1 line (header)" "1" "$lines"

# --- hash -d (remove single) ---
echo ""
echo "--- hash -d name (remove one) ---"

got=$(printf 'hash ls\nhash cat\nhash -d ls\nhash\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "hash -d ls → cat remains" "/usr/bin/cat" "$got"
if echo "$got" | grep -q "/usr/bin/ls"; then
  printf "  \033[31m✗\033[0m %-45s\n" "hash -d ls → ls removed"
  FAIL=$((FAIL+1))
else
  printf "  \033[32m✓\033[0m %-45s\n" "hash -d ls → ls removed"
  PASS=$((PASS+1))
fi

# --- hash nonexistent command ---
echo ""
echo "--- hash nonexistent_cmd ---"

got=$(printf 'hash nonexistent_cmd_xyz\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "hash nonexistent → error exit 1" "1" "$got"

# --- hash exit code ---
echo ""
echo "--- Exit codes ---"

got=$(printf 'hash\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "hash (no args) → exit 0" "0" "$got"

got=$(printf 'hash ls\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "hash ls → exit 0" "0" "$got"

got=$(printf 'hash -r\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "hash -r → exit 0" "0" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

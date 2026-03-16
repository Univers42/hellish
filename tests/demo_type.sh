#!/bin/bash
# ============================================================
#  DEMO: type builtin (with alias + hash integration)
# ============================================================
SHELL_BIN="${1:-/home/dylan/sh42/build/bin/hellish}"
PASS=0; FAIL=0

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
echo "  DEMO: type builtin (alias+hash aware)"
echo "=========================================="

# --- type for builtins ---
echo ""
echo "--- Builtins ---"

got=$(printf 'type echo\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type echo → is a shell builtin" "builtin" "$got"

got=$(printf 'type cd\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type cd → is a shell builtin" "builtin" "$got"

got=$(printf 'type test\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type test → is a shell builtin" "builtin" "$got"

got=$(printf 'type alias\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type alias → is a shell builtin" "builtin" "$got"

got=$(printf 'type hash\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type hash → is a shell builtin" "builtin" "$got"

got=$(printf 'type jobs\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type jobs → is a shell builtin" "builtin" "$got"

got=$(printf 'type fg\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type fg → is a shell builtin" "builtin" "$got"

got=$(printf 'type bg\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type bg → is a shell builtin" "builtin" "$got"

got=$(printf 'type fc\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type fc → is a shell builtin" "builtin" "$got"

# --- type for aliases ---
echo ""
echo "--- Aliases ---"

got=$(printf 'alias ll="ls -la"\ntype ll\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type ll → aliased to 'ls -la'" "aliased" "$got"

got=$(printf 'alias greet="echo hi"\ntype greet\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type greet → aliased to 'echo hi'" "aliased" "$got"

# --- type for hashed commands ---
echo ""
echo "--- Hashed commands ---"

got=$(printf 'hash ls\ntype ls\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type ls (hashed) → is hashed" "hashed" "$got"

got=$(printf 'hash cat\ntype cat\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type cat (hashed) → is hashed" "hashed" "$got"

# --- type for external commands (not hashed) ---
echo ""
echo "--- External commands ---"

got=$(printf 'type /bin/ls\n' | "$SHELL_BIN" 2>/dev/null)
check_contains "type /bin/ls → is ..." "is" "$got"

# --- type for nonexistent ---
echo ""
echo "--- Nonexistent ---"

got=$(printf 'type nonexistent_xyz\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
if [ "$got" = "1" ]; then
  printf "  \033[32m✓\033[0m %-50s\n" "type nonexistent → exit 1"
  PASS=$((PASS+1))
else
  printf "  \033[31m✗\033[0m %-50s\n    expected: 1, got: %s\n" "type nonexistent → exit 1" "$got"
  FAIL=$((FAIL+1))
fi

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

#!/bin/bash
# ============================================================
#  DEMO: alias / unalias builtins
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

echo ""
echo "=========================================="
echo "  DEMO: alias / unalias"
echo "=========================================="

# --- Basic alias set and print ---
echo ""
echo "--- Setting and printing aliases ---"

got=$(printf 'alias ll="ls -la"\nalias\n' | "$SHELL_BIN" 2>/dev/null)
check "alias ll='ls -la' then list" "alias ll='ls -la'" "$got"

got=$(printf 'alias ll="ls -la"\nalias ll\n' | "$SHELL_BIN" 2>/dev/null)
check "alias ll (print single)" "alias ll='ls -la'" "$got"

# --- Multiple aliases ---
echo ""
echo "--- Multiple aliases ---"

got=$(printf 'alias a="echo AAA"\nalias b="echo BBB"\nalias\n' | "$SHELL_BIN" 2>/dev/null | sort)
expected=$(printf "alias a='echo AAA'\nalias b='echo BBB'" | sort)
check "set two aliases, list both" "$expected" "$got"

# --- Alias expansion ---
echo ""
echo "--- Alias expansion ---"

got=$(printf 'alias greet="echo hello world"\ngreet\n' | "$SHELL_BIN" 2>/dev/null)
check "alias greet → 'echo hello world'" "hello world" "$got"

got=$(printf 'alias say="echo"\nsay goodbye\n' | "$SHELL_BIN" 2>/dev/null)
check "alias say=echo, then 'say goodbye'" "goodbye" "$got"

got=$(printf 'alias myls="ls"\nmyls /dev/null\n' | "$SHELL_BIN" 2>/dev/null)
check "alias myls=ls, expand to real command" "/dev/null" "$got"

# --- Alias with arguments preserved ---
echo ""
echo "--- Arguments preserved after expansion ---"

got=$(printf 'alias e="echo"\ne one two three\n' | "$SHELL_BIN" 2>/dev/null)
check "alias e=echo; e one two three" "one two three" "$got"

# --- Alias override ---
echo ""
echo "--- Override existing alias ---"

got=$(printf 'alias x="echo first"\nalias x="echo second"\nx\n' | "$SHELL_BIN" 2>/dev/null)
check "override alias (first → second)" "second" "$got"

# --- unalias ---
echo ""
echo "--- unalias ---"

got=$(printf 'alias x="echo hi"\nunalias x\nalias\n' | "$SHELL_BIN" 2>/dev/null)
check "unalias x → empty alias list" "" "$got"

# --- unalias nonexistent ---
got=$(printf 'unalias nonexistent\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "unalias nonexistent → error exit 1" "1" "$got"

# --- unalias -a ---
echo ""
echo "--- unalias -a (remove all) ---"

got=$(printf 'alias a="echo 1"\nalias b="echo 2"\nalias c="echo 3"\nunalias -a\nalias\n' | "$SHELL_BIN" 2>/dev/null)
check "unalias -a removes all aliases" "" "$got"

# --- Alias not found ---
echo ""
echo "--- Edge cases ---"

got=$(printf 'alias nonexistent 2>&1\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "alias nonexistent → error exit 1" "1" "$got"

# --- No-arg alias (empty list) ---
got=$(printf 'alias\n' | "$SHELL_BIN" 2>/dev/null)
check "alias with no aliases defined → empty" "" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

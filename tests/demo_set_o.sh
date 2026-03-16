#!/bin/bash
# ============================================================
#  DEMO: set -o vi / set -o emacs (editing modes)
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
echo "  DEMO: set -o vi / set -o emacs"
echo "=========================================="

# --- Default mode (emacs) ---
echo ""
echo "--- Default mode ---"

got=$(printf 'set -o\n' | "$SHELL_BIN" 2>/dev/null)
vi_line=$(echo "$got" | grep "^vi")
emacs_line=$(echo "$got" | grep "^emacs")
check "default: vi off" "vi	off" "$vi_line"
check "default: emacs on" "emacs	on" "$emacs_line"

# --- Switch to vi ---
echo ""
echo "--- Switch to vi mode ---"

got=$(printf 'set -o vi\nset -o\n' | "$SHELL_BIN" 2>/dev/null)
vi_line=$(echo "$got" | grep "^vi")
emacs_line=$(echo "$got" | grep "^emacs")
check "after set -o vi: vi on" "vi	on" "$vi_line"
check "after set -o vi: emacs off" "emacs	off" "$emacs_line"

# --- Switch back to emacs ---
echo ""
echo "--- Switch back to emacs ---"

got=$(printf 'set -o vi\nset -o emacs\nset -o\n' | "$SHELL_BIN" 2>/dev/null)
vi_line=$(echo "$got" | grep "^vi")
emacs_line=$(echo "$got" | grep "^emacs")
check "after set -o emacs: vi off" "vi	off" "$vi_line"
check "after set -o emacs: emacs on" "emacs	on" "$emacs_line"

# --- set +o (show current mode, same output) ---
echo ""
echo "--- set +o (show settings) ---"

got=$(printf 'set +o\n' | "$SHELL_BIN" 2>/dev/null)
if echo "$got" | grep -q "emacs\|vi"; then
  printf "  \033[32m✓\033[0m %-50s\n" "set +o shows vi/emacs settings"
  PASS=$((PASS+1))
else
  printf "  \033[31m✗\033[0m %-50s\n    got: [%s]\n" "set +o shows vi/emacs settings" "$got"
  FAIL=$((FAIL+1))
fi

# --- Multiple toggles ---
echo ""
echo "--- Multiple toggles ---"

got=$(printf 'set -o vi\nset -o emacs\nset -o vi\nset -o emacs\nset -o\n' | "$SHELL_BIN" 2>/dev/null)
vi_line=$(echo "$got" | grep "^vi")
emacs_line=$(echo "$got" | grep "^emacs")
check "4 toggles → back to emacs: vi off" "vi	off" "$vi_line"
check "4 toggles → back to emacs: emacs on" "emacs	on" "$emacs_line"

# --- set -o with invalid option ---
echo ""
echo "--- Invalid option ---"

got=$(printf 'set -o nonexistent\necho $?\n' | "$SHELL_BIN" 2>/dev/null | tail -1)
check "set -o nonexistent → exit (!= 0)" "1" "$got"

echo ""
echo "=========================================="
printf "  RESULTS: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n" "$PASS" "$FAIL"
echo "=========================================="
echo ""

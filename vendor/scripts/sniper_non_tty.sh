#!/bin/sh
# Simple sniper to test non-tty input handling for minishell
# Usage: ./scripts/sniper_non_tty.sh

MINISHELL="./minishell"
set -eu

print_header() {
  printf "\n==== %s ===\n" "$1"
}

print_output() {
  printf '%s\n' '--- RAW OUTPUT START ---'
  printf '%s\n' "$1"
  printf '%s\n' '--- RAW OUTPUT END ---'
}

echo "Running non-tty tests against: $MINISHELL"

# Test -c invocation
print_header "Test: -c invocation"
cmd="$MINISHELL -c \"echo 'hello'\""
printf "Command: %s\n" "$cmd"
out=$($MINISHELL -c "echo 'hello'" 2>&1 || true)
print_output "$out"
if [ "$out" != "hello" ]; then
  echo "FAIL -c: expected 'hello', got:"; printf '%s\n' "$out"
  exit 1
fi
printf "OK -c\n"

# Test pipe input
print_header "Test: pipe input"
# feed an actual newline (avoid literal backslash-n in variable)
printf "Feeding input:\n%s\n" "echo hello"
out=$(printf "echo hello\n" | $MINISHELL 2>&1 || true)
print_output "$out"
if ! printf "%s\n" "$out" | grep -xq "hello"; then
  echo "FAIL pipe: expected 'hello' in output"; printf '%s\n' "$out"
  exit 1
fi
printf "OK pipe\n"

# Test file input
print_header "Test: file input"
tmpfile=$(mktemp /tmp/minishell_test.XXXX)
printf "echo filetest\n" > "$tmpfile"
printf "Using file: %s\n" "$tmpfile"
printf "File contents:\n"; cat "$tmpfile"
out=$($MINISHELL "$tmpfile" 2>&1 || true)
print_output "$out"
if ! printf "%s\n" "$out" | grep -xq "filetest"; then
  echo "FAIL file: expected 'filetest' in output"; printf '%s\n' "$out"
  rm -f "$tmpfile"
  exit 1
fi
printf "OK file\n"
rm -f "$tmpfile"

# Test heredoc / multi-line pipe
print_header "Test: heredoc / multi-line pipe"
printf "Feeding heredoc with two printf lines\n"
out=$(cat <<'EOF' | $MINISHELL 2>&1 || true
printf "a\n"
printf "b\n"
EOF
)
print_output "$out"
if ! printf "%s\n" "$out" | grep -xq "a" || ! printf "%s\n" "$out" | grep -xq "b"; then
  echo "FAIL heredoc: expected lines 'a' and 'b' in output"; printf '%s\n' "$out"
  exit 1
fi
printf "OK heredoc\n"

printf "All non-tty tests passed.\n"
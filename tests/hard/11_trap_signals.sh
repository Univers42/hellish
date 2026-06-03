#!/bin/sh
# Trap + signal handling, deterministically: EXIT traps, signal-to-self with a
# fixed handler, trap reset/ignore/list, traps in functions and subshells,
# cleanup ordering. No pids/timestamps in output. Exercises trap, kill -SIG $$,
# functions, subshells, set -e interaction.
set -u

echo "=== EXIT trap fires once at end ==="
( trap 'echo "exit-trap ran"' EXIT; echo "body"; )
echo "after subshell"

echo "=== signal to self with handler ==="
trap 'echo "caught USR1"' USR1
kill -USR1 $$
echo "after USR1"

echo "=== handler can be replaced ==="
trap 'echo "handler v2"' USR1
kill -USR1 $$
echo "after replace"

echo "=== ignore then restore ==="
trap '' USR2
kill -USR2 $$
echo "USR2 ignored (still here)"
trap 'echo "USR2 restored"' USR2
kill -USR2 $$
echo "after USR2"

echo "=== reset to default (- ) ==="
trap 'echo "INT handler"' INT
kill -INT $$ 2>/dev/null || true
echo "after INT handled"
trap - INT
echo "INT reset to default"

echo "=== counted handler via global ==="
hits=0
trap 'hits=$((hits+1))' USR1
kill -USR1 $$
kill -USR1 $$
kill -USR1 $$
echo "hits=$hits"

echo "=== trap set inside a function persists ==="
setup() { trap 'echo "func-set EXIT"' USR1; }
setup
kill -USR1 $$
echo "after func-set signal"

echo "=== EXIT trap with cleanup of a temp file ==="
tmp=$(mktemp)
cleanup() { rm -f "$tmp"; echo "cleaned up"; }
trap cleanup EXIT
echo "data" > "$tmp"
echo "temp has: $(cat "$tmp")"
echo "exists before exit: $([ -f "$tmp" ] && echo yes || echo no)"

echo "=== subshell EXIT trap is independent ==="
trap 'echo "outer EXIT"' EXIT
( trap 'echo "inner EXIT"' EXIT; echo "in inner" )
echo "back in outer"

echo "=== ordering: multiple signals queued ==="
order=""
trap 'order="${order}A"' USR1
trap 'order="${order}B"' USR2
kill -USR1 $$
kill -USR2 $$
kill -USR1 $$
echo "order=$order"
echo "done"

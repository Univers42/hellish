#!/bin/sh
# Create a private temp directory, populate it, then clean up via an EXIT trap.
# Only deterministic facts are printed (never the random temp path), so the
# output matches across runs and shells.
work=$(mktemp -d 2>/dev/null) || { echo "mktemp failed"; exit 1; }
trap 'rm -rf "$work"' EXIT INT TERM
i=1
while [ $i -le 5 ]; do echo "line $i" > "$work/f$i.txt"; i=$((i + 1)); done
count=$(ls "$work" | wc -l)
total=$(cat "$work"/*.txt | wc -l)
echo "files created: $count"
echo "total lines: $total"
echo "third file content: $(cat "$work/f3.txt")"

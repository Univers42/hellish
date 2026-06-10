#!/bin/sh
# Pure-shell wc: count lines, words and characters of generated text using
# only read, set -- field splitting and ${#}; no external commands.
tmp=$(mktemp) || exit 1
i=0
while [ $i -lt 800 ]; do
	printf 'the quick brown fox %d jumps over the lazy dog\n' "$i"
	i=$((i+1))
done > "$tmp"

lines=0
words=0
chars=0
while IFS= read -r line; do
	lines=$((lines + 1))
	chars=$((chars + ${#line} + 1))
	set -- $line
	words=$((words + $#))
done < "$tmp"
rm -f "$tmp"

echo "$lines $words $chars"

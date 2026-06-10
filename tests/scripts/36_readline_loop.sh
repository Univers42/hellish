#!/bin/sh
# while-read line processing: generate a 2000-line file with a pure shell
# loop, then stream it back with `read`, counting lines and summing the
# numeric field. Exercises the read builtin + redirection in a tight loop.
tmp=$(mktemp) || exit 1
i=0
while [ $i -lt 2000 ]; do
	printf 'row %d end\n' "$i"
	i=$((i+1))
done > "$tmp"

lines=0
sum=0
while IFS= read -r line; do
	set -- $line
	sum=$((sum + $2))
	lines=$((lines + 1))
done < "$tmp"
rm -f "$tmp"

echo "lines=$lines"
echo "sum=$sum"

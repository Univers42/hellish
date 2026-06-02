#!/bin/sh
# Redirections into/out of a private tmp dir: >, >>, <, and combining.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

echo "first line" > "$work/file.txt"
echo "second line" >> "$work/file.txt"
printf 'third\nfourth\n' >> "$work/file.txt"

echo "-- file contents --"
cat "$work/file.txt"

echo "-- line count via redirect-in --"
wc -l < "$work/file.txt"

echo "-- sorted --"
sort < "$work/file.txt"

# truncate then append
: > "$work/file.txt"
echo "fresh" >> "$work/file.txt"
echo "-- after truncate --"
cat "$work/file.txt"

# write multiple files and concatenate
for i in 1 2 3; do
	echo "data-$i" > "$work/part-$i"
done
cat "$work/part-1" "$work/part-2" "$work/part-3"

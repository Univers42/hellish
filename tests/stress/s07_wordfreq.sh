#!/bin/sh
# Word-frequency histogram through a classic pipeline, then a `while read` loop
# that reassembles the columns. LC_ALL=C keeps sort ordering stable/portable.
text="the quick brown fox the lazy dog the fox jumps over the lazy dog the end"
printf '%s\n' "$text" | tr ' ' '\n' | LC_ALL=C sort | uniq -c \
	| LC_ALL=C sort -rn -k1,1 -k2,2 | while read -r count word; do
	bar=""
	i=0
	while [ $i -lt "$count" ]; do bar="$bar#"; i=$((i + 1)); done
	printf '%-6s %2d %s\n' "$word" "$count" "$bar"
done

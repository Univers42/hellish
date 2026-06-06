#!/bin/sh
# Read a numeric grid from a here-document into a pseudo 2-D array m_<r>_<c>
# (heredoc-fed `while read`, so it runs in the current shell and the array
# survives), then print the transpose.
r=0; cols=0
while read -r line; do
	[ -z "$line" ] && continue
	c=0
	for v in $line; do eval "m_${r}_${c}=$v"; c=$((c + 1)); done
	cols=$c; r=$((r + 1))
done <<'GRID'
1 2 3 4
5 6 7 8
9 10 11 12
GRID
rows=$r
j=0
while [ $j -lt $cols ]; do
	line=""; i=0
	while [ $i -lt $rows ]; do
		v=$(eval "printf '%s' \"\$m_${i}_${j}\"")
		line="$line $v"; i=$((i + 1))
	done
	echo "${line# }"
	j=$((j + 1))
done

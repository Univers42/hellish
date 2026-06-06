#!/bin/sh
# Draw an ASCII diamond of N rows (N odd), computing padding and star counts.
N=9
mid=$((N / 2))
i=0
while [ $i -lt $N ]; do
	if [ $i -le $mid ]; then stars=$((2 * i + 1)); else stars=$((2 * (N - 1 - i) + 1)); fi
	pad=$(((N - stars) / 2))
	line=""; p=0
	while [ $p -lt $pad ]; do line="$line "; p=$((p + 1)); done
	s=0
	while [ $s -lt $stars ]; do line="$line*"; s=$((s + 1)); done
	echo "$line"
	i=$((i + 1))
done

#!/bin/sh
# Collatz: among 1..N, find the start with the longest hailstone sequence.
N=100
best=0; bestlen=0
i=1
while [ $i -le $N ]; do
	n=$i; len=0
	while [ "$n" -ne 1 ]; do
		if [ $((n % 2)) -eq 0 ]; then n=$((n / 2)); else n=$((3 * n + 1)); fi
		len=$((len + 1))
	done
	if [ "$len" -gt "$bestlen" ]; then bestlen=$len; best=$i; fi
	i=$((i + 1))
done
printf 'longest Collatz under %s: n=%s len=%s\n' "$N" "$best" "$bestlen"

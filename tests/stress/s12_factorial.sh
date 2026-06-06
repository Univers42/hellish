#!/bin/sh
# Factorials 1..20 (20! is the largest that still fits a signed 64-bit int).
# A good probe for the shell's arithmetic width.
f=1
n=1
while [ $n -le 20 ]; do
	f=$((f * n))
	printf '%2d! = %s\n' "$n" "$f"
	n=$((n + 1))
done

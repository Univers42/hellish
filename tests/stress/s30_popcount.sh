#!/bin/sh
# Binary representation + population count (set-bit count) of several integers.
popcount() {
	n=$1; c=0
	while [ $n -gt 0 ]; do c=$((c + n % 2)); n=$((n / 2)); done
	echo $c
}
tobin() {
	n=$1; b=""
	if [ $n -eq 0 ]; then echo 0; return; fi
	while [ $n -gt 0 ]; do b="$((n % 2))$b"; n=$((n / 2)); done
	echo "$b"
}
for n in 0 1 7 8 255 1024 4095; do
	printf '%5d = %-12s popcount=%d\n' "$n" "$(tobin $n)" "$(popcount $n)"
done

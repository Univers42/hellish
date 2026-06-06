#!/bin/sh
# Narcissistic numbers up to 1000 where the sum of the cubes of the digits
# equals the number itself.
n=1
res=""
while [ $n -le 1000 ]; do
	t=$n; sum=0
	while [ $t -gt 0 ]; do
		d=$((t % 10))
		sum=$((sum + d * d * d))
		t=$((t / 10))
	done
	[ $sum -eq $n ] && res="$res $n"
	n=$((n + 1))
done
echo "narcissistic (cube) numbers <= 1000:$res"

#!/bin/sh
# Perfect numbers up to 10000: those equal to the sum of their proper divisors.
n=2
res=""
while [ $n -le 10000 ]; do
	sum=1; d=2
	while [ $((d * d)) -le $n ]; do
		if [ $((n % d)) -eq 0 ]; then
			sum=$((sum + d))
			[ $((d * d)) -ne $n ] && sum=$((sum + n / d))
		fi
		d=$((d + 1))
	done
	[ $sum -eq $n ] && res="$res $n"
	n=$((n + 1))
done
echo "perfect numbers <= 10000:$res"

#!/bin/sh
# Binomial coefficients C(n,k), computed iteratively to avoid large factorials.
ncr() {
	n=$1; k=$2; r=1; i=1
	[ $k -gt $((n - k)) ] && k=$((n - k))
	while [ $i -le $k ]; do
		r=$((r * (n - i + 1) / i))
		i=$((i + 1))
	done
	echo "$r"
}
for n in 5 8 10 12; do
	row=""; k=0
	while [ $k -le $n ]; do row="$row $(ncr $n $k)"; k=$((k + 1)); done
	printf 'C(%d,*):%s\n' "$n" "$row"
done

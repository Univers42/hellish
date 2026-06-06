#!/bin/sh
# Prime factorization by trial division up to sqrt(n).
factorize() {
	n=$1; out=""; d=2
	while [ $((d * d)) -le $n ]; do
		while [ $((n % d)) -eq 0 ]; do out="$out $d"; n=$((n / d)); done
		d=$((d + 1))
	done
	[ $n -gt 1 ] && out="$out $n"
	echo "${out# }"
}
for n in 2 12 100 97 360 1024 5040 999983; do
	printf '%6d = %s\n' "$n" "$(factorize $n)"
done

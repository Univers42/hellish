#!/bin/sh
# Integer -> Roman numeral via a value/symbol table walked with `shift 2`.
to_roman() {
	n=$1; res=""
	set -- 1000 M 900 CM 500 D 400 CD 100 C 90 XC 50 L 40 XL 10 X 9 IX 5 V 4 IV 1 I
	while [ $# -gt 0 ]; do
		val=$1; sym=$2; shift 2
		while [ "$n" -ge "$val" ]; do res="$res$sym"; n=$((n - val)); done
	done
	echo "$res"
}
for x in 4 9 14 40 90 400 944 1994 2026 3888; do
	printf '%s = %s\n' "$x" "$(to_roman $x)"
done

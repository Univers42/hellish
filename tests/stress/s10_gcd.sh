#!/bin/sh
# Euclid's GCD + LCM over pairs re-split from a single string with `set --`.
gcd() {
	a=$1; b=$2
	while [ "$b" -ne 0 ]; do t=$b; b=$((a % b)); a=$t; done
	echo "$a"
}
lcm() {
	g=$(gcd "$1" "$2")
	echo $(($1 / g * $2))
}
for pair in "12 18" "48 36" "17 5" "100 75" "1071 462" "0 5"; do
	set -- $pair
	printf 'gcd(%s,%s)=%s lcm=%s\n' "$1" "$2" "$(gcd $1 $2)" "$(lcm $1 $2)"
done

#!/bin/sh
# Euclid's GCD (iterative and recursive) plus LCM.
gcd_iter() {
	a=$1
	b=$2
	while [ "$b" -ne 0 ]; do
		t=$((a % b))
		a=$b
		b=$t
	done
	echo "$a"
}

gcd_rec() {
	if [ "$2" -eq 0 ]; then
		echo "$1"
	else
		gcd_rec "$2" $(($1 % $2))
	fi
}

for pair in "48 18" "100 75" "17 5" "1071 462"; do
	# shellcheck disable=SC2086
	set -- $pair
	g1=$(gcd_iter "$1" "$2")
	g2=$(gcd_rec "$1" "$2")
	lcm=$(( $1 / g1 * $2 ))
	echo "gcd($1,$2)=$g1/$g2 lcm=$lcm"
done

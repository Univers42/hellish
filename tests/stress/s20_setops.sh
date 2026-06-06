#!/bin/sh
# Set operations (intersection, difference) over two integer lists using a
# membership helper that re-splits its list argument.
A="1 3 5 7 9 11"
B="3 6 9 12"
member() {
	needle=$1; shift
	for e in "$@"; do [ "$e" -eq "$needle" ] && return 0; done
	return 1
}
inter=""
for x in $A; do member "$x" $B && inter="$inter $x"; done
only_a=""
for x in $A; do member "$x" $B || only_a="$only_a $x"; done
only_b=""
for y in $B; do member "$y" $A || only_b="$only_b $y"; done
printf 'A      : %s\n' "$A"
printf 'B      : %s\n' "$B"
printf 'A and B: %s\n' "${inter# }"
printf 'A - B  : %s\n' "${only_a# }"
printf 'B - A  : %s\n' "${only_b# }"

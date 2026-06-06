#!/bin/sh
# Decimal -> arbitrary base (2..16). Digit lookup via `cut -c` indexing into a
# digit string. Stresses nested command substitution inside a loop.
to_base() {
	n=$1; b=$2; digs=0123456789abcdef; res=""
	if [ "$n" -eq 0 ]; then echo 0; return; fi
	while [ "$n" -gt 0 ]; do
		d=$((n % b))
		res="$(printf '%s' "$digs" | cut -c$((d + 1)))$res"
		n=$((n / b))
	done
	echo "$res"
}
for n in 0 8 16 100 255 4096 65535; do
	printf '%s: bin=%s oct=%s hex=%s\n' "$n" "$(to_base $n 2)" "$(to_base $n 8)" "$(to_base $n 16)"
done

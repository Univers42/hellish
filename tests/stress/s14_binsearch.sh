#!/bin/sh
# Binary search over a sorted list held in positional parameters; the midpoint
# element is reached by indirect positional expansion `\${$mid}`.
search() {
	target=$1; shift
	lo=1; hi=$#
	while [ $lo -le $hi ]; do
		mid=$(((lo + hi) / 2))
		eval "v=\${$mid}"
		if [ "$v" -eq "$target" ]; then echo "$target -> index $mid"; return; fi
		if [ "$v" -lt "$target" ]; then lo=$((mid + 1)); else hi=$((mid - 1)); fi
	done
	echo "$target -> not found"
}
for t in 23 2 91 50 8 72; do
	search $t 2 5 8 12 16 23 38 56 72 91
done

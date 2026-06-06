#!/bin/sh
# Towers of Hanoi: recursion mutating a shared counter (functions run in the
# current shell, so the global persists across recursive calls).
moves=0
hanoi() {
	local n from to via
	n=$1; from=$2; to=$3; via=$4
	if [ "$n" -eq 1 ]; then
		moves=$((moves + 1))
		printf 'move disk 1 from %s to %s\n' "$from" "$to"
		return
	fi
	hanoi $((n - 1)) "$from" "$via" "$to"
	moves=$((moves + 1))
	printf 'move disk %s from %s to %s\n' "$n" "$from" "$to"
	hanoi $((n - 1)) "$via" "$to" "$from"
}
hanoi 3 A C B
echo "total moves: $moves"

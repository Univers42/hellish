#!/bin/sh
# Recursive quicksort over positional parameters. Stresses recursion + nested
# command substitution + word splitting of the partitions.
quicksort() {
	if [ $# -le 1 ]; then echo "$*"; return; fi
	pivot=$1; shift
	less=""; greater=""
	for x in "$@"; do
		if [ "$x" -lt "$pivot" ]; then less="$less $x"; else greater="$greater $x"; fi
	done
	echo "$(quicksort $less) $pivot $(quicksort $greater)"
}
quicksort 5 3 8 1 9 2 7 4 6 0
quicksort 42 17 99 8 23 4 55 1
quicksort 7
quicksort

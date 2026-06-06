#!/bin/sh
# Horizontal bar chart from label/value pairs re-split with `set --`.
for pair in "Jan 7" "Feb 12" "Mar 4" "Apr 18" "May 9" "Jun 15" "Jul 0"; do
	set -- $pair
	label=$1; val=$2
	bar=""; i=0
	while [ $i -lt "$val" ]; do bar="$bar*"; i=$((i + 1)); done
	printf '%s | %-20s %d\n' "$label" "$bar" "$val"
done

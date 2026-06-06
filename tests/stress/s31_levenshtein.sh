#!/bin/sh
# Levenshtein edit distance via dynamic programming on an eval 2-D array d_i_j.
lev() {
	a=$1; b=$2; la=${#a}; lb=${#b}
	i=0
	while [ $i -le $la ]; do eval "d_${i}_0=$i"; i=$((i + 1)); done
	j=0
	while [ $j -le $lb ]; do eval "d_0_$j=$j"; j=$((j + 1)); done
	i=1
	while [ $i -le $la ]; do
		ca=$(printf '%s' "$a" | cut -c$i)
		j=1
		while [ $j -le $lb ]; do
			cb=$(printf '%s' "$b" | cut -c$j)
			if [ "$ca" = "$cb" ]; then cost=0; else cost=1; fi
			up=$(eval "printf '%s' \"\$d_$((i - 1))_$j\"")
			left=$(eval "printf '%s' \"\$d_${i}_$((j - 1))\"")
			diag=$(eval "printf '%s' \"\$d_$((i - 1))_$((j - 1))\"")
			m=$((up + 1))
			[ $((left + 1)) -lt $m ] && m=$((left + 1))
			[ $((diag + cost)) -lt $m ] && m=$((diag + cost))
			eval "d_${i}_${j}=$m"
			j=$((j + 1))
		done
		i=$((i + 1))
	done
	eval "printf '%s\n' \"\$d_${la}_${lb}\""
}
for pair in "kitten sitting" "flaw lawn" "gumbo gambol" "abc abc" "x yyyy"; do
	set -- $pair
	printf '%-8s %-8s -> %s\n' "$1" "$2" "$(lev "$1" "$2")"
done

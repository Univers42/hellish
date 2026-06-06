#!/bin/sh
# Longest common subsequence length via DP on an eval array c_i_j.
lcs() {
	a=$1; b=$2; la=${#a}; lb=${#b}
	i=0
	while [ $i -le $la ]; do eval "c_${i}_0=0"; i=$((i + 1)); done
	j=0
	while [ $j -le $lb ]; do eval "c_0_$j=0"; j=$((j + 1)); done
	i=1
	while [ $i -le $la ]; do
		ca=$(printf '%s' "$a" | cut -c$i)
		j=1
		while [ $j -le $lb ]; do
			cb=$(printf '%s' "$b" | cut -c$j)
			diag=$(eval "printf '%s' \"\$c_$((i - 1))_$((j - 1))\"")
			up=$(eval "printf '%s' \"\$c_$((i - 1))_$j\"")
			left=$(eval "printf '%s' \"\$c_${i}_$((j - 1))\"")
			if [ "$ca" = "$cb" ]; then
				v=$((diag + 1))
			else
				v=$up; [ $left -gt $v ] && v=$left
			fi
			eval "c_${i}_${j}=$v"
			j=$((j + 1))
		done
		i=$((i + 1))
	done
	eval "printf '%s\n' \"\$c_${la}_${lb}\""
}
for pair in "ABCBDAB BDCAB" "AGGTAB GXTXAYB" "abc abc" "abc xyz"; do
	set -- $pair
	printf 'LCS(%s,%s) = %s\n' "$1" "$2" "$(lcs "$1" "$2")"
done

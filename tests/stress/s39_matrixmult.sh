#!/bin/sh
# Multiply two 3x3 matrices held in eval arrays a_i_j and b_i_j.
set_m() { eval "$1_${2}_${3}=$4"; }
get_m() { eval "printf '%s' \"\$$1_${2}_${3}\""; }
r=0
for row in "1 2 3" "4 5 6" "7 8 9"; do
	c=0
	for v in $row; do set_m a $r $c $v; c=$((c + 1)); done
	r=$((r + 1))
done
r=0
for row in "2 0 1" "0 2 0" "1 0 2"; do
	c=0
	for v in $row; do set_m b $r $c $v; c=$((c + 1)); done
	r=$((r + 1))
done
i=0
while [ $i -lt 3 ]; do
	line=""; j=0
	while [ $j -lt 3 ]; do
		sum=0; k=0
		while [ $k -lt 3 ]; do
			sum=$((sum + $(get_m a $i $k) * $(get_m b $k $j)))
			k=$((k + 1))
		done
		line="$line $(printf '%3d' $sum)"
		j=$((j + 1))
	done
	echo "${line# }"
	i=$((i + 1))
done

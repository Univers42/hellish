#!/bin/sh
# Pascal's triangle. Each new row is built in n_<k> from the previous row in
# c_<k>, then copied back. Stresses eval pseudo-arrays + arithmetic.
N=9
eval "c_0=1"
echo "1"
r=1
while [ $r -lt $N ]; do
	line="1"; eval "n_0=1"
	k=1
	while [ $k -lt $r ]; do
		a=$(eval "printf '%s' \"\$c_$((k - 1))\"")
		b=$(eval "printf '%s' \"\$c_$k\"")
		v=$((a + b)); eval "n_$k=$v"; line="$line $v"; k=$((k + 1))
	done
	eval "n_$r=1"; line="$line 1"
	echo "$line"
	k=0
	while [ $k -le $r ]; do
		v=$(eval "printf '%s' \"\$n_$k\""); eval "c_$k=$v"; k=$((k + 1))
	done
	r=$((r + 1))
done

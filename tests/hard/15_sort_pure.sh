#!/bin/sh
# Pure-shell insertion sort of 400 LCG-generated numbers. No arrays, no
# externals: the sorted sequence lives in one string and every insert walks
# it with for/IFS splitting -- a worst case for word splitting + concat.
seed=42
next() {
	seed=$(( (seed * 1103515245 + 12345) % 2147483648 ))
	rnd=$(( seed % 1000 ))
}

sorted=
n=0
while [ $n -lt 400 ]; do
	next
	out=
	placed=0
	for v in $sorted; do
		if [ $placed -eq 0 ] && [ $rnd -lt $v ]; then
			out="$out $rnd"
			placed=1
		fi
		out="$out $v"
	done
	if [ $placed -eq 0 ]; then
		out="$out $rnd"
	fi
	sorted=$out
	n=$((n+1))
done

set -- $sorted
echo "count=$#"
echo "first=$1"
min=$1
sum=0
for v in $sorted; do
	sum=$((sum + v))
	max=$v
done
echo "min=$min max=$max sum=$sum"

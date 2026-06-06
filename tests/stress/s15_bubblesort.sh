#!/bin/sh
# Bubble sort over an eval-array a_0..a_{n-1}, swapping in place.
set -- 5 2 9 1 7 3 8 4 6 0
n=0
for x in "$@"; do eval "a_$n=$x"; n=$((n + 1)); done
i=0
while [ $i -lt $((n - 1)) ]; do
	j=0
	while [ $j -lt $((n - 1 - i)) ]; do
		aj=$(eval "printf '%s' \"\$a_$j\"")
		ak=$(eval "printf '%s' \"\$a_$((j + 1))\"")
		if [ "$aj" -gt "$ak" ]; then
			eval "a_$j=$ak"; eval "a_$((j + 1))=$aj"
		fi
		j=$((j + 1))
	done
	i=$((i + 1))
done
out=""; k=0
while [ $k -lt $n ]; do
	out="$out $(eval "printf '%s' \"\$a_$k\"")"; k=$((k + 1))
done
echo "sorted:$out"

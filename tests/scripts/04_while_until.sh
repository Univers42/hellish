#!/bin/sh
# while and until loops, break and continue.
i=0
while [ "$i" -lt 5 ]; do
	i=$((i + 1))
	if [ "$i" -eq 3 ]; then
		continue
	fi
	echo "while i=$i"
done

j=10
until [ "$j" -le 5 ]; do
	echo "until j=$j"
	j=$((j - 1))
done

# break out early
k=0
while true; do
	k=$((k + 1))
	echo "loop $k"
	if [ "$k" -ge 4 ]; then
		break
	fi
done
echo "final k=$k"

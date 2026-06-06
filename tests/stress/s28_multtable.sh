#!/bin/sh
# 9x9 multiplication table with right-aligned columns built by concatenating
# printf field outputs.
i=1
while [ $i -le 9 ]; do
	line=""; j=1
	while [ $j -le 9 ]; do
		line="$line$(printf '%4d' $((i * j)))"; j=$((j + 1))
	done
	echo "$line"
	i=$((i + 1))
done

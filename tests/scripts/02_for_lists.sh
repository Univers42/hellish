#!/bin/sh
# for loops over literal lists, glob-free word lists and ranges via seq.
for fruit in apple banana cherry; do
	echo "fruit: $fruit"
done

total=0
for n in 1 2 3 4 5; do
	total=$((total + n))
done
echo "sum 1..5 = $total"

# for over command substitution output (word splitting on whitespace)
for w in $(echo "one two three"); do
	echo "word=[$w]"
done

# nested for
for i in 1 2 3; do
	for j in a b; do
		echo "$i$j"
	done
done

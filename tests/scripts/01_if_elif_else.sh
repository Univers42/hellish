#!/bin/sh
# Basic if / elif / else chains driven by arithmetic and string tests.
classify() {
	n=$1
	if [ "$n" -lt 0 ]; then
		echo "negative"
	elif [ "$n" -eq 0 ]; then
		echo "zero"
	elif [ "$n" -lt 10 ]; then
		echo "small"
	else
		echo "big"
	fi
}

for v in -5 0 3 42; do
	echo "$v -> $(classify "$v")"
done

word=hello
if [ "$word" = "hello" ]; then
	echo "greeting matched"
fi

if [ -z "$word" ]; then
	echo "empty"
else
	echo "non-empty word of length ${#word}"
fi

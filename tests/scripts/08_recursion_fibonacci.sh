#!/bin/sh
# Recursive Fibonacci (exponential but small) and an iterative cross-check.
fib() {
	if [ "$1" -lt 2 ]; then
		echo "$1"
	else
		a=$(fib $(($1 - 1)))
		b=$(fib $(($1 - 2)))
		echo $((a + b))
	fi
}

echo "-- recursive --"
for n in 0 1 2 3 4 5 6 7 8; do
	printf 'fib(%d)=%s\n' "$n" "$(fib "$n")"
done

echo "-- iterative --"
a=0
b=1
i=0
while [ "$i" -lt 9 ]; do
	echo "$a"
	t=$((a + b))
	a=$b
	b=$t
	i=$((i + 1))
done

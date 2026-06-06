#!/bin/sh
# Fibonacci with a memo array f_<n> filled iteratively, then queried.
f_0=0; f_1=1
i=2
while [ $i -le 40 ]; do
	a=$(eval "printf '%s' \"\$f_$((i - 1))\"")
	b=$(eval "printf '%s' \"\$f_$((i - 2))\"")
	eval "f_$i=$((a + b))"
	i=$((i + 1))
done
for n in 0 1 2 10 20 30 40; do
	printf 'fib(%s)=%s\n' "$n" "$(eval "printf '%s' \"\$f_$n\"")"
done

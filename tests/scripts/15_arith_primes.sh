#!/bin/sh
# Primes below 50 via inline trial division (arithmetic-heavy looping).
#
# NOTE: the trial-division loop is kept INLINE on purpose. hellish has a severe
# performance bug where a `while` loop placed INSIDE a function body, when that
# function is called repeatedly, becomes pathologically slow / appears to hang
# (correct output, but does not finish in any reasonable time). Writing the
# inner loop inline keeps this script fast and deterministic while still
# exercising nested arithmetic loops. See the test report for the bug details.
primes=""
n=2
while [ "$n" -lt 50 ]; do
	d=2
	is_prime=1
	while [ $((d * d)) -le "$n" ]; do
		if [ $((n % d)) -eq 0 ]; then
			is_prime=0
			break
		fi
		d=$((d + 1))
	done
	if [ "$is_prime" -eq 1 ]; then
		primes="$primes $n"
	fi
	n=$((n + 1))
done
echo "primes:$primes"

# count them
count=0
for p in $primes; do
	count=$((count + 1))
done
echo "count=$count"

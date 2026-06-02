#!/bin/sh
# Arithmetic-heavy loops: sums, factorial-by-loop, power.
sum=0
i=1
while [ "$i" -le 100 ]; do
	sum=$((sum + i))
	i=$((i + 1))
done
echo "sum 1..100 = $sum"

sq=0
for n in 1 2 3 4 5 6 7 8 9 10; do
	sq=$((sq + n * n))
done
echo "sum of squares 1..10 = $sq"

# power via loop
base=2
exp=10
result=1
c=0
while [ "$c" -lt "$exp" ]; do
	result=$((result * base))
	c=$((c + 1))
done
echo "$base^$exp = $result"

# triangular numbers
for t in 1 2 3 4 5; do
	echo "T($t)=$(( t * (t + 1) / 2 ))"
done

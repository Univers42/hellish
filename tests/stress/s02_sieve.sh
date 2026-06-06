#!/bin/sh
# Sieve of Eratosthenes using eval-built pseudo-array p_<n>. Stresses eval +
# indirect variable access + arithmetic in tight loops.
N=80
i=2
while [ $i -le $N ]; do eval "p_$i=1"; i=$((i + 1)); done
i=2
while [ $((i * i)) -le $N ]; do
	if eval "[ \$p_$i -eq 1 ]"; then
		j=$((i * i))
		while [ $j -le $N ]; do eval "p_$j=0"; j=$((j + i)); done
	fi
	i=$((i + 1))
done
out=""
i=2
while [ $i -le $N ]; do
	if eval "[ \$p_$i -eq 1 ]"; then out="$out $i"; fi
	i=$((i + 1))
done
echo "primes<=$N:$out"

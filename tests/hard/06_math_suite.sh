#!/bin/sh
# Heavy integer math: factorial, fibonacci (iter + recursive), gcd/lcm, primes
# (trial + sieve via positional set), base conversion, modular exponentiation,
# Collatz, perfect numbers. Exercises recursion, arithmetic (all operators,
# precedence, bases), functions returning via echo, set -- as an array.
set -u

fact() { n=$1; r=1; while [ "$n" -gt 1 ]; do r=$((r*n)); n=$((n-1)); done; echo "$r"; }

fib_iter() {
	n=$1; a=0; b=1; i=0
	while [ "$i" -lt "$n" ]; do t=$((a+b)); a=$b; b=$t; i=$((i+1)); done
	echo "$a"
}
fib_rec() {
	if [ "$1" -lt 2 ]; then echo "$1"; return; fi
	echo $(( $(fib_rec $(($1-1))) + $(fib_rec $(($1-2))) ))
}

gcd() { a=$1; b=$2; while [ "$b" -ne 0 ]; do t=$((a%b)); a=$b; b=$t; done; echo "$a"; }
lcm() { g=$(gcd "$1" "$2"); echo $(( $1 / g * $2 )); }

is_prime() {
	n=$1
	[ "$n" -lt 2 ] && return 1
	i=2
	while [ $((i*i)) -le "$n" ]; do
		[ $((n%i)) -eq 0 ] && return 1
		i=$((i+1))
	done
	return 0
}

modpow() { # base exp mod
	b=$1; e=$2; m=$3; r=1; b=$((b%m))
	while [ "$e" -gt 0 ]; do
		if [ $((e%2)) -eq 1 ]; then r=$(( r*b%m )); fi
		e=$((e/2)); b=$(( b*b%m ))
	done
	echo "$r"
}

collatz_steps() {
	n=$1; s=0
	while [ "$n" -ne 1 ]; do
		if [ $((n%2)) -eq 0 ]; then n=$((n/2)); else n=$((3*n+1)); fi
		s=$((s+1))
	done
	echo "$s"
}

to_base() { # num base -> digits string
	num=$1; base=$2; out=""
	[ "$num" -eq 0 ] && { echo 0; return; }
	while [ "$num" -gt 0 ]; do
		d=$((num%base))
		case $d in
			1[0-5]) c=$(printf '\\%o' $((87+d)) ); out="$(printf "$c")$out" ;;
			*) out="$d$out" ;;
		esac
		num=$((num/base))
	done
	echo "$out"
}

echo "=== factorials ==="
for k in 0 1 5 10 13; do printf 'fact(%d)=%s\n' "$k" "$(fact "$k")"; done

echo "=== fibonacci (iter) ==="
i=0; while [ "$i" -le 15 ]; do printf '%s ' "$(fib_iter "$i")"; i=$((i+1)); done; echo

echo "=== fibonacci (recursive) check matches iter ==="
ok=1
i=0; while [ "$i" -le 12 ]; do
	[ "$(fib_rec "$i")" = "$(fib_iter "$i")" ] || ok=0
	i=$((i+1))
done
echo "fib_rec_matches=$ok"

echo "=== gcd / lcm ==="
for pair in "48 36" "100 75" "17 5" "1071 462"; do
	set -- $pair
	printf 'gcd(%d,%d)=%s lcm=%s\n' "$1" "$2" "$(gcd "$1" "$2")" "$(lcm "$1" "$2")"
done

echo "=== primes up to 50 ==="
p=""
n=2
while [ "$n" -le 50 ]; do
	if is_prime "$n"; then p="$p $n"; fi
	n=$((n+1))
done
set -- $p
echo "count=$# list:$p"

echo "=== sieve via positional toggles (primes < 30) ==="
# mark[2..29]; use a string of 0/1
limit=30
marks=""
i=0; while [ "$i" -le "$limit" ]; do marks="${marks}1"; i=$((i+1)); done
i=2
while [ $((i*i)) -le "$limit" ]; do
	# if marks[i]==1, cross out multiples
	pre=$(printf '%s' "$marks" | cut -c$((i+1)))
	if [ "$pre" = 1 ]; then
		j=$((i*i))
		while [ "$j" -le "$limit" ]; do
			marks="$(printf '%s' "$marks" | cut -c1-$j)0$(printf '%s' "$marks" | cut -c$((j+2))-)"
			j=$((j+i))
		done
	fi
	i=$((i+1))
done
out=""
i=2; while [ "$i" -le "$limit" ]; do
	b=$(printf '%s' "$marks" | cut -c$((i+1)))
	[ "$b" = 1 ] && out="$out $i"
	i=$((i+1))
done
echo "sieve:$out"

echo "=== modular exponentiation ==="
printf '2^10 mod 1000 = %s\n' "$(modpow 2 10 1000)"
printf '3^20 mod 50 = %s\n' "$(modpow 3 20 50)"
printf '7^256 mod 13 = %s\n' "$(modpow 7 256 13)"

echo "=== collatz steps ==="
for n in 6 7 27 97; do printf 'collatz(%d)=%s\n' "$n" "$(collatz_steps "$n")"; done

echo "=== base conversion ==="
for spec in "255 16" "255 2" "100 8" "0 16" "1000 16" "31 2"; do
	set -- $spec
	printf '%d base%d = %s\n' "$1" "$2" "$(to_base "$1" "$2")"
done

echo "=== arithmetic precedence + bases drill ==="
echo "$(( 2 + 3 * 4 - 6 / 2 ))"
echo "$(( (2 + 3) * 4 ))"
echo "$(( 2 ** 3 ** 2 ))"
echo "$(( 0xff + 0x01 ))"
echo "$(( 010 + 7 ))"
echo "$(( 1 << 10 ))"
echo "$(( 255 & 0x0f ))"
echo "$(( 5 > 3 ? 100 : 200 ))"
echo "$(( ~0 ))"
echo "$(( 17 % 5 + 3 * 2 ))"
echo "done"

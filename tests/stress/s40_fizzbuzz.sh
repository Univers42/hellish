#!/bin/sh
# FizzBuzz 1..30 driven by a case over the two divisibility remainders.
i=1
while [ $i -le 30 ]; do
	a=$((i % 3)); b=$((i % 5))
	case "$a$b" in
	00) echo FizzBuzz ;;
	0*) echo Fizz ;;
	*0) echo Buzz ;;
	*) echo "$i" ;;
	esac
	i=$((i + 1))
done

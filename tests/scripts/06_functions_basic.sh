#!/bin/sh
# Function definitions, arguments, return codes, $0/$1 inside functions.
greet() {
	echo "hello, $1"
}

add() {
	echo $(($1 + $2))
}

check_positive() {
	if [ "$1" -gt 0 ]; then
		return 0
	else
		return 1
	fi
}

greet world
echo "2+3=$(add 2 3)"

if check_positive 5; then
	echo "5 is positive"
fi
if check_positive -2; then
	echo "unexpected"
else
	echo "-2 is not positive (rc=$?)"
fi

show_args() {
	echo "fn argc=$#"
	echo "fn arg1=$1 arg2=$2"
}
show_args alpha beta gamma

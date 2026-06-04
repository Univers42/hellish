#!/bin/sh
# Mutual recursion: even/odd via return codes.
is_even() {
	if [ "$1" -eq 0 ]; then
		return 0
	fi
	is_odd $(($1 - 1))
}

is_odd() {
	if [ "$1" -eq 0 ]; then
		return 1
	fi
	is_even $(($1 - 1))
}

for n in 0 1 2 3 4 7 10 11; do
	if is_even "$n"; then
		echo "$n even"
	else
		echo "$n odd"
	fi
done

#!/bin/sh
# Recursive factorial computed via arithmetic and command substitution.
factorial() {
	if [ "$1" -le 1 ]; then
		echo 1
	else
		prev=$(factorial $(($1 - 1)))
		echo $(($1 * prev))
	fi
}

for n in 0 1 5 7 10; do
	echo "$n! = $(factorial "$n")"
done

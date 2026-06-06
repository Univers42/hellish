#!/bin/sh
# Ackermann function: deep, doubly-recursive. The arguments are `local` so the
# nested calls do not clobber each other.
ack() {
	local m n
	m=$1; n=$2
	if [ "$m" -eq 0 ]; then echo $((n + 1)); return; fi
	if [ "$n" -eq 0 ]; then ack $((m - 1)) 1; return; fi
	inner=$(ack "$m" $((n - 1)))
	ack $((m - 1)) "$inner"
}
for pair in "0 0" "1 1" "2 3" "3 3" "2 5"; do
	set -- $pair
	printf 'ack(%s,%s) = %s\n' "$1" "$2" "$(ack $1 $2)"
done

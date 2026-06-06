#!/bin/sh
# A DFA that accepts binary strings whose value is divisible by 3. The state is
# the running value mod 3; transition is state' = (2*state + bit) mod 3, encoded
# as a case over "<state><bit>".
accepts() {
	s=$1; state=0
	while [ -n "$s" ]; do
		c=${s%"${s#?}"}; s=${s#?}
		case "$state$c" in
		00) state=0 ;; 01) state=1 ;;
		10) state=2 ;; 11) state=0 ;;
		20) state=1 ;; 21) state=2 ;;
		esac
	done
	[ "$state" -eq 0 ] && echo accept || echo reject
}
for b in 0 11 110 1001 1010 111111 100100; do
	printf '%-8s -> %s\n' "$b" "$(accepts "$b")"
done

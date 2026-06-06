#!/bin/sh
# Reverse a string one character at a time using only parameter expansion
# (${s#?} drops the first char; ${s%"${s#?}"} keeps just the first char), then
# test for palindrome.
reverse() {
	s=$1; r=""
	while [ -n "$s" ]; do
		c=${s%"${s#?}"}
		r="$c$r"
		s=${s#?}
	done
	echo "$r"
}
for w in level hello racecar shell noon abcba abca x; do
	rev=$(reverse "$w")
	if [ "$w" = "$rev" ]; then v=yes; else v=no; fi
	printf '%-8s rev=%-8s palindrome=%s\n' "$w" "$rev" "$v"
done

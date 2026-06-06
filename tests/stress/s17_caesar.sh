#!/bin/sh
# ROT13 Caesar cipher via tr with a rotated alphabet; it is its own inverse, so
# encode then decode must round-trip.
rot() { printf '%s' "$1" | tr 'A-Za-z' 'N-ZA-Mn-za-m'; }
for msg in "Hello, World" "The Quick Brown Fox" "abcXYZ 123"; do
	e=$(rot "$msg")
	d=$(rot "$e")
	printf 'plain=[%s] cipher=[%s] back=[%s]\n' "$msg" "$e" "$d"
done

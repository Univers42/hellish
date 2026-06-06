#!/bin/sh
# Run-length encode a string (char by char), then decode to verify round-trip.
encode() {
	s=$1; out=""; prev=""; count=0
	while [ -n "$s" ]; do
		c=${s%"${s#?}"}; s=${s#?}
		if [ "$c" = "$prev" ]; then
			count=$((count + 1))
		else
			[ -n "$prev" ] && out="$out$count$prev"
			prev=$c; count=1
		fi
	done
	[ -n "$prev" ] && out="$out$count$prev"
	echo "$out"
}
decode() {
	s=$1; out=""; num=""
	while [ -n "$s" ]; do
		c=${s%"${s#?}"}; s=${s#?}
		case "$c" in
		[0-9]) num="$num$c" ;;
		*)
			i=0
			while [ $i -lt "$num" ]; do out="$out$c"; i=$((i + 1)); done
			num=""
			;;
		esac
	done
	echo "$out"
}
for w in aaabbbcccd aabbccdd xxxxyz abc; do
	e=$(encode "$w")
	d=$(decode "$e")
	printf '%-12s -> %-12s -> %s\n' "$w" "$e" "$d"
done

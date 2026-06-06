#!/bin/sh
# Group words that are anagrams by their sorted-letter signature.
sig() { printf '%s' "$1" | fold -w1 | LC_ALL=C sort | tr -d '\n'; }
words="listen silent enlist cat act dog god tac"
seen=""
for w in $words; do
	s=$(sig "$w")
	group=""
	for x in $words; do
		[ "$(sig "$x")" = "$s" ] && group="$group $x"
	done
	case " $seen " in
	*" $s "*) ;;
	*) seen="$seen $s"; printf '%s:%s\n' "$s" "$group" ;;
	esac
done

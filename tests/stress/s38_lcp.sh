#!/bin/sh
# Longest common prefix of a word list, shrinking the candidate with a case
# glob match against each word.
lcp() {
	prefix=$1; shift
	for w in "$@"; do
		while [ -n "$prefix" ]; do
			case "$w" in
			"$prefix"*) break ;;
			*) prefix=${prefix%?} ;;
			esac
		done
	done
	echo "$prefix"
}
printf 'lcp1=[%s]\n' "$(lcp flower flow flight)"
printf 'lcp2=[%s]\n' "$(lcp dog cat racecar)"
printf 'lcp3=[%s]\n' "$(lcp interspecies interstellar interstate)"
printf 'lcp4=[%s]\n' "$(lcp abc abc abc)"

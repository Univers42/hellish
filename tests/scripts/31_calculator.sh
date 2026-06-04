#!/bin/sh
# A small RPN-ish calculator driven by positional args; deterministic args
# are supplied by the runner. Demonstrates args + arithmetic + dispatch.
#
# Usage as run by harness: script.sh 10 plus 5 times 3
acc=0
op="plus"
first=1

apply() {
	val=$1
	if [ "$first" -eq 1 ]; then
		acc=$val
		first=0
		return
	fi
	case "$op" in
		plus) acc=$((acc + val)) ;;
		minus) acc=$((acc - val)) ;;
		times) acc=$((acc * val)) ;;
		div) acc=$((acc / val)) ;;
		mod) acc=$((acc % val)) ;;
	esac
}

for tok in "$@"; do
	case "$tok" in
		plus | minus | times | div | mod)
			op=$tok
			;;
		*)
			apply "$tok"
			;;
	esac
done

echo "result=$acc"

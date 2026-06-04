#!/bin/sh
# getopts option parsing with a flag, an option-argument, and leftovers.
verbose=0
name=""
count=1

while getopts "vn:c:" opt; do
	case "$opt" in
		v) verbose=$((verbose + 1)) ;;
		n) name=$OPTARG ;;
		c) count=$OPTARG ;;
		*) echo "bad option" ;;
	esac
done
shift $((OPTIND - 1))

echo "verbose=$verbose"
echo "name=$name"
echo "count=$count"
echo "remaining=$*"

i=0
while [ "$i" -lt "$count" ]; do
	echo "tick $i"
	i=$((i + 1))
done

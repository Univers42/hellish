#!/bin/sh
# Positional parameters, shift, and "$@" vs "$*" behavior under IFS.
echo "start argc=$#"

while [ "$#" -gt 0 ]; do
	echo "head=$1 remaining=$#"
	shift
done
echo "after drain argc=$#"

set -- one two three four
echo "reset argc=$#"
shift 2
echo "after shift 2: $1 $2 (argc=$#)"

set -- a b c
echo "star-default=[$*]"
old_ifs=$IFS
IFS=-
echo "star-dash=[$*]"
IFS=$old_ifs
echo "at-quoted-count: $# args"
for x in "$@"; do
	echo "x=$x"
done

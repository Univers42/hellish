#!/bin/sh
# for loop iterating over "$@" and bare $@, plus arg count.
echo "argc=$#"
echo "args=[$*]"

i=1
for a in "$@"; do
	echo "arg$i=$a"
	i=$((i + 1))
done

# implicit "for x do" iterates over "$@"
echo "-- implicit --"
for x do
	echo "implicit:$x"
done

#!/bin/sh
# All permutations of a short string. Recursive, so the working variables are
# declared `local` to survive the recursion intact.
perms() {
	local prefix rest i n c left right
	prefix=$1; rest=$2
	if [ -z "$rest" ]; then echo "$prefix"; return; fi
	n=${#rest}; i=1
	while [ $i -le $n ]; do
		c=$(printf '%s' "$rest" | cut -c$i)
		if [ $i -le 1 ]; then left=""; else left=$(printf '%s' "$rest" | cut -c1-$((i - 1))); fi
		right=$(printf '%s' "$rest" | cut -c$((i + 1))-)
		perms "$prefix$c" "$left$right"
		i=$((i + 1))
	done
}
perms "" "abc"
echo "--- count of perms(1234):"
perms "" "1234" | wc -l

#!/bin/sh
# Parameter-expansion string toolkit over a generated word list: prefix and
# suffix trimming, length, defaults, and ${var//pat/rep} substitution.
# Pure expansions, no externals -- per-word ${} cost dominates.
i=0
words=
while [ $i -lt 200 ]; do
	words="$words /usr/lib/mod_$i/file_$i.tar.gz"
	i=$((i+1))
done

total=0
longest=
for p in $words; do
	base=${p##*/}
	dir=${p%/*}
	stem=${base%%.*}
	ext=${base#*.}
	dashed=${stem//_/-}
	total=$((total + ${#dashed}))
	if [ ${#dir} -gt ${#longest} ]; then
		longest=$dir
	fi
	unset empty
	mark=${empty:-none}
done
echo "total=$total"
echo "longest=$longest"
echo "last=$dashed.$ext"
echo "mark=$mark"

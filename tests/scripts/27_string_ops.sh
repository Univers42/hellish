#!/bin/sh
# String manipulation using POSIX expansions and tools (no bash ${x/y/z}).
sentence="the quick brown fox"

# word count
set -- $sentence
echo "words=$#"

# reverse words
rev=""
for w in $sentence; do
	rev="$w $rev"
done
echo "reversed: $rev"

# uppercase first word via tr
first=${sentence%% *}
echo "first upper: $(printf '%s' "$first" | tr 'a-z' 'A-Z')"

# replace spaces with dashes using tr
echo "dashed: $(printf '%s' "$sentence" | tr ' ' '-')"

# count characters
echo "char count = ${#sentence}"

# strip a known prefix/suffix
greeting="Hello, World!"
echo "no-hello: ${greeting#Hello, }"
echo "no-bang:  ${greeting%!}"

# build a CSV from a loop
csv=""
for n in 1 2 3 4; do
	if [ -z "$csv" ]; then
		csv=$n
	else
		csv="$csv,$n"
	fi
done
echo "csv=$csv"

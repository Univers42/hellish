#!/bin/sh
# Nested and chained parameter expansions plus indirect-ish patterns.
prefix=log
suffix=txt
sep=.
filename="${prefix}${sep}${suffix}"
echo "built: $filename"
echo "stem:  ${filename%"$sep$suffix"}"

base=${filename%.*}
ext=${filename##*.}
echo "base=$base ext=$ext"

# Default that itself uses another expansion
unset color
fallback=blue
echo "${color:-${fallback}}"

# length of a substituted default
echo "len-of-default=${#color}"
color=${color:-red}
echo "len-after=${#color}"

# combine with arithmetic
count=7
echo "padded=$((count * 100 + 5))"
echo "msg=${prefix}-$((count + 1))"

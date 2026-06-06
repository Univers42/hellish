#!/bin/sh
# IFS manipulation: split records on different delimiters (including empty
# fields from a non-whitespace IFS) and rejoin with $*.
record="alpha:beta:gamma:delta"
oldIFS=$IFS
IFS=:
set -- $record
IFS=$oldIFS
echo "fields: $#"
n=1
for f in "$@"; do printf '  [%d] %s\n' "$n" "$f"; n=$((n + 1)); done
line="1,2,,4"
IFS=,
set -- $line
IFS=$oldIFS
printf 'csv count=%d ->' "$#"
for f in "$@"; do printf ' <%s>' "$f"; done
printf '\n'
IFS=-
echo "joined: $*"
IFS=$oldIFS

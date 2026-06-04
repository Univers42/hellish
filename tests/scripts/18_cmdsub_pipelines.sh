#!/bin/sh
# Command substitution feeding pipelines with classic text tools.
nums=$(seq 1 10)
echo "seq words: $nums"

evens=$(echo "$nums" | awk '$1 % 2 == 0')
echo "evens:"
echo "$evens"

count=$(echo "$nums" | wc -l)
echo "line count = $count"

reversed=$(seq 1 5 | sort -rn | paste -sd, -)
echo "reversed-csv: $reversed"

upper=$(echo "hello world" | tr 'a-z' 'A-Z')
echo "upper: $upper"

third=$(printf 'a:b:c:d\n' | cut -d: -f3)
echo "third field: $third"

# nested command substitution
echo "nested: $(echo $(echo deep))"

#!/bin/sh
# Word frequency counter over a fixed text block: tokenize, sort, count.
text=$(cat <<'EOF'
the cat sat on the mat
the dog sat on the log
the cat and the dog
EOF
)

echo "-- frequency (sorted by word) --"
printf '%s\n' "$text" | tr ' ' '\n' | sort | uniq -c | awk '{print $2": "$1}'

echo "-- top word --"
printf '%s\n' "$text" | tr ' ' '\n' | sort | uniq -c | sort -rn | head -1 | awk '{print $2" ("$1")"}'

echo "-- total words --"
printf '%s\n' "$text" | wc -w

echo "-- distinct words --"
printf '%s\n' "$text" | tr ' ' '\n' | sort -u | wc -l

#!/bin/sh
# Multi-stage text processing pipeline over a here-doc data block.
data=$(cat <<'EOF'
alice 30 engineer
bob 25 artist
carol 35 engineer
dave 28 artist
eve 40 engineer
EOF
)

echo "-- names sorted --"
echo "$data" | awk '{print $1}' | sort

echo "-- engineers --"
echo "$data" | grep engineer | awk '{print $1}'

echo "-- average age --"
echo "$data" | awk '{s += $2; n++} END {print s / n}'

echo "-- oldest --"
echo "$data" | sort -k2 -rn | head -1 | awk '{print $1, $2}'

echo "-- count by job --"
echo "$data" | awk '{print $3}' | sort | uniq -c | awk '{print $2": "$1}'

#!/bin/sh
# Formatted table report generator with printf alignment, computed columns,
# borders, totals, and number formatting. Exercises: printf width/precision,
# arithmetic, functions, here-docs, while-read, parameter expansion, sort.
set -u

rows() {
	cat <<-'DATA'
		Widget|120|4.50
		Gadget|75|12.00
		Gizmo|200|2.25
		Doohickey|15|99.99
		Thingamajig|9|150.00
		Sprocket|340|0.75
	DATA
}

hr() {
	w=$1
	i=0
	printf '+'
	while [ "$i" -lt "$w" ]; do printf '-'; i=$((i+1)); done
	printf '+\n'
}

echo "=== inventory table ==="
hr 44
printf '|%-14s|%8s|%9s|%9s|\n' "Item" "Qty" "Price" "Value"
hr 44
grand=0
tot_qty=0
while IFS='|' read -r name qty price; do
	# price is dollars.cents; compute value = qty*price in cents
	cents=$(printf '%s' "$price" | tr -d '.')
	# strip leading zeros for arithmetic
	cents=$((10#$cents))
	val_cents=$(( qty * cents ))
	dollars=$(( val_cents / 100 ))
	rem=$(( val_cents % 100 ))
	printf '|%-14s|%8d|%9s|%6d.%02d|\n' "$name" "$qty" "$price" "$dollars" "$rem"
	grand=$(( grand + val_cents ))
	tot_qty=$(( tot_qty + qty ))
done <<EOF
$(rows)
EOF
hr 44
printf '|%-14s|%8d|%9s|%6d.%02d|\n' "TOTAL" "$tot_qty" "" $((grand/100)) $((grand%100))
hr 44

echo "=== sorted by qty (desc) ==="
rows | sort -t'|' -k2,2nr | while IFS='|' read -r name qty price; do
	printf '%-14s %4d\n' "$name" "$qty"
done

echo "=== histogram of qty (scaled) ==="
rows | while IFS='|' read -r name qty price; do
	bars=$(( qty / 20 ))
	b=""
	i=0; while [ "$i" -lt "$bars" ]; do b="$b#"; i=$((i+1)); done
	printf '%-14s %s\n' "$name" "$b"
done

echo "=== number formatting drills ==="
printf '[%5d]\n' 42
printf '[%-5d]\n' 42
printf '[%05d]\n' 42
printf '[%+d]\n' 42
printf '[%x] [%X] [%o]\n' 255 255 8
printf '[%8.3f]\n' 3.14159
printf '[%-10s|%10s]\n' left right
printf '[%c%c%c]\n' 65 66 67
printf '%d%%\n' 50
printf 'hex of 3735928559 = %x\n' 3735928559

echo "=== aligned key-value report ==="
print_kv() { printf '  %-12s : %s\n' "$1" "$2"; }
print_kv "host" "localhost"
print_kv "port" "8080"
print_kv "uptime" "$((3600 * 25 + 130)) seconds"
print_kv "ratio" "$(( 1000 * 7 / 3 )) (x1000)"
echo "done"

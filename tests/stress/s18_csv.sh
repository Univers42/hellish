#!/bin/sh
# Parse CSV from a here-document with `IFS=, read`, computing a running total.
total=0
while IFS=, read -r name qty price; do
	[ "$name" = "name" ] && continue
	sub=$((qty * price))
	total=$((total + sub))
	printf '%-8s %3d x %3d = %d\n' "$name" "$qty" "$price" "$sub"
done <<'CSV'
name,qty,price
apple,3,40
bread,2,25
milk,5,15
eggs,12,5
CSV
echo "grand total: $total"

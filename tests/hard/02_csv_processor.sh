#!/bin/sh
# CSV record processor: parse, filter, transform, aggregate, format a report.
# Exercises: IFS field splitting, read -r, while-read from heredoc, arithmetic
# (incl. integer division/modulo), printf alignment, case, functions, "$@".
set -u

emp_data() {
	cat <<-'CSV'
		id,name,dept,salary,years
		1,Alice,eng,9500,5
		2,Bob,sales,6200,3
		3,Carol,eng,11000,8
		4,Dave,ops,5400,2
		5,Eve,sales,7300,6
		6,Frank,eng,8800,4
		7,Grace,ops,6000,7
		8,Heidi,sales,9100,9
		9,Ivan,eng,12500,11
		10,Judy,ops,4800,1
	CSV
}

echo "=== raw row count (excluding header) ==="
n=$(emp_data | tail -n +2 | wc -l | tr -d ' ')
echo "rows=$n"

echo "=== employees by dept (sorted), with salary ==="
emp_data | tail -n +2 | sort -t, -k3,3 -k4,4n | while IFS=, read -r id name dept salary years; do
	printf '%-3s %-8s %-6s $%-6s %syr\n' "$id" "$name" "$dept" "$salary" "$years"
done

echo "=== per-dept aggregates ==="
# accumulate sum/count/max per dept into positional-ish strings
for dept in eng ops sales; do
	sum=0; cnt=0; max=0; maxn=""
	while IFS=, read -r id name d salary years; do
		[ "$d" = "$dept" ] || continue
		sum=$(( sum + salary ))
		cnt=$(( cnt + 1 ))
		if [ "$salary" -gt "$max" ]; then
			max=$salary
			maxn=$name
		fi
	done <<EOF
$(emp_data | tail -n +2)
EOF
	if [ "$cnt" -gt 0 ]; then
		avg=$(( sum / cnt ))
		rem=$(( (sum % cnt) * 100 / cnt ))
		printf 'dept=%-6s n=%-2d sum=%-6d avg=%d.%02d top=%s($%d)\n' \
			"$dept" "$cnt" "$sum" "$avg" "$rem" "$maxn" "$max"
	fi
done

echo "=== salary bands (case on ranges) ==="
band() {
	s=$1
	if [ "$s" -lt 6000 ]; then echo low
	elif [ "$s" -lt 9000 ]; then echo mid
	elif [ "$s" -lt 12000 ]; then echo high
	else echo exec
	fi
}
low=0; mid=0; high=0; exec_c=0
emp_data | tail -n +2 | while IFS=, read -r id name dept salary years; do
	b=$(band "$salary")
	echo "$name:$b"
done | sort -t: -k2

echo "=== count per band (subshell accumulation via here-string-ish) ==="
counts=$(emp_data | tail -n +2 | while IFS=, read -r id name dept salary years; do
	band "$salary"
done | sort | uniq -c | while read -r c b; do printf '%s=%s ' "$b" "$c"; done)
echo "bands: $counts"

echo "=== raise simulation: +10% rounded, total payroll before/after ==="
before=0; after=0
emp_data | tail -n +2 > /tmp/csv_rows_$$ 2>/dev/null || emp_data | tail -n +2 > /tmp/csv_rows
rows_file=/tmp/csv_rows_$$
[ -f "$rows_file" ] || rows_file=/tmp/csv_rows
while IFS=, read -r id name dept salary years; do
	before=$(( before + salary ))
	raise=$(( salary * 110 / 100 ))
	after=$(( after + raise ))
done < "$rows_file"
rm -f "$rows_file"
echo "payroll_before=$before payroll_after=$after delta=$(( after - before ))"

echo "=== field extraction with cut + paste-like join ==="
emp_data | tail -n +2 | cut -d, -f2,4 | sort -t, -k2,2nr | head -3 | while IFS=, read -r name sal; do
	printf 'top-earner %s: %s\n' "$name" "$sal"
done
echo "done"

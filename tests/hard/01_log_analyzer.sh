#!/bin/sh
# Synthetic log generator + analyzer. Deterministic (no time/pid/random in output).
# Exercises: functions, recursion, local-ish via positional, case, while-read,
# heredocs (plain/quoted/dash, in functions and loops), parameter expansion,
# arithmetic, command substitution, pipes, redirections, IFS, traps, set -u.

set -u
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
raw="$work/raw.log"
: > "$raw"

LEVELS="DEBUG INFO WARN ERROR FATAL"

# deterministic pseudo-random based on a linear congruential generator in $((..))
seed=12345
nextrand() {
	seed=$(( (seed * 1103515245 + 12345) & 2147483647 ))
	echo $(( seed % $1 ))
}

level_for() {
	# map 0..99 to a level with a fixed distribution
	n=$1
	if [ "$n" -lt 40 ]; then echo INFO
	elif [ "$n" -lt 65 ]; then echo DEBUG
	elif [ "$n" -lt 85 ]; then echo WARN
	elif [ "$n" -lt 97 ]; then echo ERROR
	else echo FATAL
	fi
}

gen_logs() {
	count=$1
	i=0
	while [ "$i" -lt "$count" ]; do
		r=$(nextrand 100)
		lvl=$(level_for "$r")
		svc=$(( i % 4 ))
		case $svc in
			0) src=auth ;;
			1) src=db ;;
			2) src=web ;;
			*) src=cache ;;
		esac
		code=$(( 200 + (r % 5) * 100 ))
		printf '%04d|%s|%s|code=%d|msg=request_%d\n' "$i" "$lvl" "$src" "$code" "$i"
		i=$(( i + 1 ))
	done
}

gen_logs 200 > "$raw"
echo "=== total lines ==="
wc -l < "$raw" | tr -d ' '

echo "=== counts by level (sorted) ==="
# pull level field (2nd, IFS=|), count, sort
cut -d'|' -f2 "$raw" | sort | uniq -c | sort -k2 | while read -r n lvl; do
	printf '%-6s %s\n' "$lvl" "$n"
done

echo "=== counts by source ==="
for src in auth cache db web; do
	c=$(grep -c "|$src|" "$raw")
	printf '%-6s %d\n' "$src" "$c"
done

echo "=== error+ lines (ERROR/FATAL), first 5 ids ==="
grep -E '\|(ERROR|FATAL)\|' "$raw" | cut -d'|' -f1 | head -5 | while read -r id; do
	printf 'err id=%s\n' "$id"
done

# Build a per-source error tally using a temp file as an associative store.
tally="$work/tally"
: > "$tally"
while IFS='|' read -r id lvl src rest; do
	case $lvl in
		ERROR|FATAL)
			# increment count for src in the tally file
			cur=$(grep "^$src " "$tally" 2>/dev/null | cut -d' ' -f2)
			cur=${cur:-0}
			new=$(( cur + 1 ))
			grep -v "^$src " "$tally" > "$tally.tmp" 2>/dev/null || :
			mv "$tally.tmp" "$tally" 2>/dev/null || :
			printf '%s %d\n' "$src" "$new" >> "$tally"
			;;
	esac
done < "$raw"

echo "=== error tally by source (sorted) ==="
sort "$tally" | while read -r src cnt; do
	printf '%s -> %s errors\n' "$src" "$cnt"
done

# Severity score: weight levels and sum (arithmetic-heavy)
weight() {
	case $1 in
		DEBUG) echo 1 ;;
		INFO)  echo 2 ;;
		WARN)  echo 5 ;;
		ERROR) echo 10 ;;
		FATAL) echo 25 ;;
		*) echo 0 ;;
	esac
}
total=0
for lvl in $LEVELS; do
	c=$(cut -d'|' -f2 "$raw" | grep -c "^$lvl$")
	w=$(weight "$lvl")
	sub=$(( c * w ))
	total=$(( total + sub ))
	printf 'level=%-5s n=%-3d w=%-2d sub=%d\n' "$lvl" "$c" "$w" "$sub"
done
echo "severity_total=$total"

# A report via a heredoc with expansion, inside a function
report() {
	name=$1
	cat <<-EOF
		---- report: $name ----
		generated lines : $(wc -l < "$raw" | tr -d ' ')
		severity total  : $total
		threshold状态    : $( [ "$total" -gt 500 ] && echo HIGH || echo LOW )
	EOF
}
report "summary"

# Quoted heredoc (no expansion)
cat <<'NOEXP'
literal: $total ${not_expanded} `echo nope`
NOEXP

# nested command substitution + arithmetic
avg_code=$(( $(cut -d'|' -f4 "$raw" | sed 's/code=//' | paste -sd+ - 2>/dev/null || cut -d'|' -f4 "$raw" | sed 's/code=//' | tr '\n' '+' | sed 's/+$//') ))
echo "avg_code_sum=$avg_code"
echo "done rc=$?"

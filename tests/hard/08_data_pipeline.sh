#!/bin/sh
# Multi-stage data pipeline: generate records, transform through pipes and
# subshells, fan results into temp files via fd redirection, aggregate, clean up
# with a trap. Exercises: pipelines, subshells () vs groups {}, fd redirection
# (>&, <&, exec fd), command substitution chains, trap EXIT, while-read, IFS.
set -u
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# Stage 1: generate deterministic numeric records.
gen() {
	i=1
	while [ "$i" -le "$1" ]; do
		printf '%d %d\n' "$i" $(( (i * 37 + 11) % 100 ))
		i=$((i+1))
	done
}

gen 30 > "$work/data"
echo "=== records ==="
wc -l < "$work/data" | tr -d ' '

echo "=== sum/min/max/avg via single pass ==="
sum=0; min=999; max=-1; cnt=0
while read -r idx val; do
	sum=$((sum+val))
	[ "$val" -lt "$min" ] && min=$val
	[ "$val" -gt "$max" ] && max=$val
	cnt=$((cnt+1))
done < "$work/data"
echo "sum=$sum min=$min max=$max avg=$((sum/cnt)) cnt=$cnt"

echo "=== pipeline: evens vs odds (by value) into separate files ==="
while read -r idx val; do
	if [ $((val%2)) -eq 0 ]; then
		echo "$val" >> "$work/even"
	else
		echo "$val" >> "$work/odd"
	fi
done < "$work/data"
printf 'even=%s odd=%s\n' \
	"$(wc -l < "$work/even" 2>/dev/null | tr -d ' ')" \
	"$(wc -l < "$work/odd" 2>/dev/null | tr -d ' ')"

echo "=== top 5 values (sort | head) ==="
cut -d' ' -f2 "$work/data" | sort -rn | head -5 | tr '\n' ' '; echo

echo "=== subshell isolation vs group ==="
x=outer
( x=inner_subshell; echo "in subshell x=$x" )
echo "after subshell x=$x"
{ x=inner_group; echo "in group x=$x"; }
echo "after group x=$x"

echo "=== fd redirection: stdout/stderr split + merge ==="
emit() { echo "to-stdout"; echo "to-stderr" >&2; }
emit > "$work/out" 2> "$work/err"
echo "out=[$(cat "$work/out")] err=[$(cat "$work/err")]"
emit 2>&1 | sort | tr '\n' ','; echo

echo "=== exec custom fd ==="
exec 3> "$work/fd3"
echo "via fd3 line1" >&3
echo "via fd3 line2" >&3
exec 3>&-
echo "fd3 file:"; cat "$work/fd3"

echo "=== read from a custom input fd ==="
exec 4< "$work/data"
read -r a b <&4
echo "first record via fd4: a=$a b=$b"
read -r a b <&4
echo "second record via fd4: a=$a b=$b"
exec 4<&-

echo "=== pipeline with cmdsub chain + arithmetic ==="
total=$(cut -d' ' -f2 "$work/data" | while read -r v; do echo "$v"; done | awk '{s+=$1} END{print s}' 2>/dev/null || cut -d' ' -f2 "$work/data" | paste -sd+ - 2>/dev/null)
echo "awk_or_paste_total=$total"

echo "=== count by bucket (0-24,25-49,50-74,75-99) ==="
b0=0; b1=0; b2=0; b3=0
while read -r idx val; do
	case $val in
		[0-9]|1[0-9]|2[0-4]) b0=$((b0+1)) ;;
		2[5-9]|3[0-9]|4[0-9]) b1=$((b1+1)) ;;
		5[0-9]|6[0-9]|7[0-4]) b2=$((b2+1)) ;;
		*) b3=$((b3+1)) ;;
	esac
done < "$work/data"
echo "buckets: [0-24]=$b0 [25-49]=$b1 [50-74]=$b2 [75-99]=$b3"
echo "done"

#!/bin/sh
# Deeply nested control flow: for inside while inside if, with case dispatch.
process() {
	mode=$1
	case "$mode" in
		grid)
			r=1
			while [ "$r" -le 3 ]; do
				line=""
				for c in 1 2 3; do
					line="$line$((r * c)) "
				done
				echo "$line"
				r=$((r + 1))
			done
			;;
		stars)
			n=1
			while [ "$n" -le 4 ]; do
				row=""
				i=0
				while [ "$i" -lt "$n" ]; do
					row="$row*"
					i=$((i + 1))
				done
				echo "$row"
				n=$((n + 1))
			done
			;;
		*)
			echo "unknown mode: $mode"
			;;
	esac
}

echo "== grid =="
process grid
echo "== stars =="
process stars
echo "== bad =="
process zzz

# FizzBuzz to 15 (classic nested-condition exercise)
echo "== fizzbuzz =="
i=1
while [ "$i" -le 15 ]; do
	if [ $((i % 15)) -eq 0 ]; then
		echo FizzBuzz
	elif [ $((i % 3)) -eq 0 ]; then
		echo Fizz
	elif [ $((i % 5)) -eq 0 ]; then
		echo Buzz
	else
		echo "$i"
	fi
	i=$((i + 1))
done

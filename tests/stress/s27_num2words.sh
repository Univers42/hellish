#!/bin/sh
# Convert 0..999 to English words. Digit groups index into word lists held in
# positional parameters via `set --` + indirect expansion.
ones="zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen"
tens="x x twenty thirty forty fifty sixty seventy eighty ninety"
word() {
	n=$1
	if [ $n -lt 20 ]; then
		set -- $ones; eval "printf '%s' \"\${$((n + 1))}\""; return
	fi
	if [ $n -lt 100 ]; then
		t=$((n / 10)); o=$((n % 10))
		set -- $tens; eval "printf '%s' \"\${$((t + 1))}\""
		if [ $o -ne 0 ]; then set -- $ones; printf -- '-'; eval "printf '%s' \"\${$((o + 1))}\""; fi
		return
	fi
	h=$((n / 100)); rem=$((n % 100))
	set -- $ones; eval "printf '%s' \"\${$((h + 1))}\""; printf ' hundred'
	if [ $rem -ne 0 ]; then printf ' '; word $rem; fi
}
for n in 0 7 13 20 42 99 100 215 777 999; do
	printf '%3d = ' "$n"; word $n; printf '\n'
done

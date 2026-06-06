#!/bin/sh
# Arbitrary-precision addition of two non-negative decimal strings, digit by
# digit with carry, so it works far beyond a 64-bit integer.
bigadd() {
	a=$1; b=$2; res=""; carry=0
	while [ -n "$a" ] || [ -n "$b" ] || [ "$carry" -ne 0 ]; do
		da=0; db=0
		if [ -n "$a" ]; then da=${a#"${a%?}"}; a=${a%?}; fi
		if [ -n "$b" ]; then db=${b#"${b%?}"}; b=${b%?}; fi
		sum=$((da + db + carry))
		res="$((sum % 10))$res"
		carry=$((sum / 10))
	done
	echo "$res"
}
printf '%s\n' "$(bigadd 999 1)"
printf '%s\n' "$(bigadd 123456789 987654321)"
printf '%s\n' "$(bigadd 99999999999999999999 1)"
printf '%s\n' "$(bigadd 0 0)"
printf '%s\n' "$(bigadd 5040 360)"

#!/bin/sh
# printf formatting: widths, padding, integer/hex/octal, repeated format reuse.
printf '%s|%s|%s\n' a b c
printf '[%5s]\n' hi
printf '[%-5s]\n' hi
printf '[%05d]\n' 42
printf '[%+d]\n' 7
printf '%d in hex is %x and octal %o\n' 255 255 255
printf '%c%c%c\n' h i '!'

# format string reused for extra args
printf '%s\n' one two three

# numeric table
i=1
while [ "$i" -le 5 ]; do
	printf '%2d squared = %3d\n' "$i" $((i * i))
	i=$((i + 1))
done

# percent literal and no trailing newline behavior
printf '100%% done'
printf '\n'

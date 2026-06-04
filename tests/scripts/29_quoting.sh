#!/bin/sh
# Quoting rules: single vs double quotes, escaping, word splitting.
x=value
echo "double: $x and literal \$x"
echo 'single: $x stays literal'
echo "mixed: '$x'"

# spaces preserved inside quotes
spaced="a   b   c"
echo "quoted spaces: [$spaced]"
echo "unquoted splits:" $spaced

# escaped characters
echo "tab-here:	end"
echo "backslash: a\\b"
printf '%s\n' 'percent literal: 100%'

# empty and special
empty=""
echo "empty=[$empty] len=${#empty}"

# quotes around command substitution preserve newlines
multi=$(printf 'one\ntwo\nthree')
echo "quoted-cmdsub:"
echo "$multi"
echo "unquoted-cmdsub:" $multi

# concatenation of quoted segments
echo "ab""cd"'ef'

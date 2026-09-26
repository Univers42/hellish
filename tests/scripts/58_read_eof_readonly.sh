# read assigns its variables even at end of input, and never a readonly one.
#
# At EOF with nothing read, hellish returned 1 without touching the
# variables, so they kept their old values. That made the common loop
#     while IFS= read -r line || [ -n "$line" ]; do ...; done
# (which also takes a last line with no newline) run forever: $line still
# held the last line. bash assigns them empty.
#
# And read wrote readonly variables: `readonly r=1; echo x | read r`
# changed r. bash refuses with "r: readonly variable" and a status.
msg() { sed 's/^.*: line [0-9]*: //'; }

echo "-- EOF with nothing read empties the variables"
x=old; y=old; REPLY=old; arr=(o l d)
printf '' | { read -r x y; echo "rc=$? [${x-unset}] [${y-unset}]"; }
printf '' | { read -r; echo "rc=$? [${REPLY-unset}]"; }
printf '' | { read -r -a arr; echo "rc=$? n=${#arr[@]}"; }
printf 'a\n' | { read -r x; read -r x; echo "rc=$? [$x]"; }
printf '' | { read -r -d : x; echo "rc=$? [$x]"; }
printf '' | { read -r -n 3 x; echo "rc=$? [$x]"; }
printf '' | { read -r -N 3 x; echo "rc=$? [$x]"; }

echo "-- a last line with no newline, then EOF"
printf 'last' | { read -r x y; echo "rc=$? [$x] [$y]"; }
printf 'l1\nl2' | {
	n=0
	while IFS= read -r line || [ -n "$line" ]; do
		n=$((n + 1)); echo "got [$line]"
		[ "$n" -gt 5 ] && { echo "runaway loop"; break; }
	done
}
printf 'k=v\nlast=1' | {
	n=0
	while IFS= read -r line || [ -n "$line" ]; do
		n=$((n + 1)); echo "line: $line"
		[ "$n" -gt 5 ] && { echo "runaway loop"; break; }
	done
	echo "done, line=[$line]"
}

echo "-- a readonly variable is refused, the others follow bash"
readonly ro=keep
echo x | { read -r ro 2>&1 | msg; }
echo x | { read -r ro 2>/dev/null; echo "only var: rc=$? [$ro]"; }
printf '' | { read -r ro 2>/dev/null; echo "only var at EOF: rc=$? [$ro]"; }
echo "a b" | { read -r ro y 2>/dev/null; echo "first of two: rc=$? [$ro] [${y-unset}]"; }
echo "a b" | { read -r y ro 2>/dev/null; echo "last of two: rc=$? [$ro] [$y]"; }
echo "a b c" | { y=; z=old; read -r y ro z 2>/dev/null; echo "middle: rc=$? [$ro] [$y] [$z]"; }
echo a | { readonly REPLY; read -r 2>/dev/null; echo "REPLY: rc=$?"; }
ra=(1 2); readonly ra
echo "p q" | { read -r -a ra 2>&1 | msg; }
echo "p q" | { read -r -a ra 2>/dev/null; echo "array: rc=$? [${ra[*]}]"; }
echo x | { read -r ro 2>/dev/null; echo "the shell goes on"; }

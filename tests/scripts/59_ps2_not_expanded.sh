# PS2 is the prompt for a continuation line, and a shell that is not
# interactive shows no prompt, so it never expands PS2. hellish renders
# PS2 for every kind of continuation line (an open quote, a backslash-
# newline, a heredoc body, a command substitution, an unfinished
# compound); this pins that it still does so only at a prompt. The probe
# is a PS2 with a side effect: each render would count in $n.
PS2='$((n+=1))> '
echo "a
b"
echo 'c
d'
echo one \
two
cat <<EOF
body
EOF
cat <<-EOF | cat
	piped
	EOF
if true
then
  echo in-if
fi
while false
do
  :
done
f() {
  echo in-f
}
f
x=$(echo sub
)
echo "$x"
y=`echo bq
`
echo "$y"
echo a &&
echo b-after-and
echo c |
cat
eval 'echo "e
f"'
d=$(mktemp -d)
printf '%s\n' 'echo "g' 'h"' 'echo i \' 'j' > "$d/src"
. "$d/src"
rm -rf "$d"
echo "n=${n-unset}"

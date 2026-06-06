#!/bin/sh
# Heredocs on simple commands nested INSIDE compound commands (if / for / while /
# function bodies) -- the construct that needs the input gatherer to read the
# whole heredoc body before parsing. This guards the nested-heredoc fix.
if true; then
	cat <<'EOF'
inside if, $no expand
EOF
fi
for x in a b c; do
	cat <<EOF
loop $x
EOF
done
i=0
while [ $i -lt 2 ]; do
	cat <<EOF
while iter $i
EOF
	i=$((i + 1))
done
emit() {
	cat <<EOF > "$1"
generated content line one
second line here
EOF
}
emit hd_compound_out.txt
cat hd_compound_out.txt
rm -f hd_compound_out.txt
if true; then
	cat <<A <<B
first heredoc (ignored)
A
second heredoc (used)
B
fi

#!/bin/sh
# Here-documents on a file descriptor other than stdin, written as a script
# rather than through eval: hellish pre-scans a script for heredoc bodies
# before parsing it, and that scan read `3<<-EOF` as `<<` with the delimiter
# `-EOF`, so everything after the body vanished. And `N<<EOF` fed the body to
# stdin whatever N said, so the loop below failed "3: Bad file descriptor".
while read -r line <&3; do
	echo "fd3: $line"
done 3<<EOF
first
second
EOF

cat 3<<-EOF <&3
	tab-stripped on fd 3
	EOF
echo "after the fd-3 dash heredoc"

reader() {
	while read -r a <&4 && read -r b <&5; do
		echo "$a $b"
	done
}
reader 4<<-LEFT 5<<RIGHT
	l1
	l2
	LEFT
r1
r2
RIGHT

cat 3<<EOF </dev/null
this must not reach stdout
EOF
echo "end of script"

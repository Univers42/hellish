#!/bin/sh
# Redirections on a function definition belong to the body and apply at every
# call (POSIX function_body: compound_command [redirect_list]). hellish reported
# the first redirect as a syntax error. Written as a script, not through eval:
# a heredoc on the definition's own line is captured from the unread input,
# a path eval never takes, and a call must re-read (and re-expand) it each time.
log=func_def_redirect.log
rm -f "$log"

quiet() {
	echo "to stderr" >&2
	echo "to stdout"
} 2>/dev/null
quiet

logged() {
	echo "call $1"
} >>"$log"
logged 1
logged 2
cat "$log"
rm -f "$log"

greet() { cat; } <<EOF
hello $name
EOF
name=first
greet
name=second
greet

tabbed() {
	cat <&3
} 3<<-END
	fd three, tabs stripped
	END
tabbed
tabbed

counter() for i in 1 2 3; do echo "item $i"; done >"$log"
counter
cat "$log"
rm -f "$log"
echo "end of script"

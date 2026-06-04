#!/bin/sh
# Subshells isolate variable changes and cwd; exit status propagates.
x=outer
(
	x=inner
	echo "inside subshell x=$x"
)
echo "outside x=$x"

# subshell exit code
(exit 7)
echo "subshell rc=$?"

# subshell cd does not affect parent
here=$(pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
(
	cd "$work" || exit 1
	echo "in subshell, pwd changed: $([ "$(pwd)" != "$here" ] && echo yes || echo no)"
)
echo "parent pwd unchanged: $([ "$(pwd)" = "$here" ] && echo yes || echo no)"

# grouping with braces runs in current shell
y=1
{ y=2; echo "in group y=$y"; }
echo "after group y=$y"

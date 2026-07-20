#!/bin/sh
# Regression net for batched non-interactive input delivery: the shell now
# hands the lexer every complete hazard-free line at once, so everything
# here must behave exactly as it did line-by-line. Exercises: comment/blank
# runs (must not clobber $?), exact $LINENO from token offsets, aliases
# defined and used across lines (hazard fallback), heredoc bodies that
# expand a function defined EARLIER in the same batch (per-range gather
# ordering), multi-line compounds at top level, backslash-newline, and a
# dot-sourced file (hazard fallback).

echo "lineno=$LINENO"

# a run of comments and blank lines between a failure and reading $?

false
# comment one
# comment two

echo "status-after-comments=$?"

f() {
	echo "body|1"
	echo "body|2"
}
while IFS='|' read -r w n; do
	echo "read w=$w n=$n"
done <<EOF
$(f)
EOF

for v in a b; do
	echo "for=$v"
done
i=2
while [ "$i" -gt 0 ]; do
	i=$((i - 1))
done
echo "while-done i=$i"
case zz in
	z*) echo "case=match" ;;
	*) echo "case=nomatch" ;;
esac

echo one\
two

alias e='echo aliased'
e ok
unalias e
e 2>/dev/null
echo "alias-gone=$?"

src="${TMPDIR:-/tmp}/batch_src_$$.sh"
printf 'sourced_var=42\nsourced_fn() { echo "sourced_fn ran"; }\n' > "$src"
. "$src"
echo "sourced_var=$sourced_var"
sourced_fn
rm -f "$src"

echo "lineno-again=$LINENO"
echo done

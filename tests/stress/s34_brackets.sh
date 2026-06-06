#!/bin/sh
# Balanced-bracket checker using a string as a stack. The top of the stack is
# its first character (${stack%"${stack#?}"}).
check() {
	s=$1; stack=""
	while [ -n "$s" ]; do
		c=${s%"${s#?}"}; s=${s#?}
		case "$c" in
		'('|'['|'{') stack="$c$stack" ;;
		')') top=${stack%"${stack#?}"}; [ "$top" = "(" ] || { echo unbalanced; return; }; stack=${stack#?} ;;
		']') top=${stack%"${stack#?}"}; [ "$top" = "[" ] || { echo unbalanced; return; }; stack=${stack#?} ;;
		'}') top=${stack%"${stack#?}"}; [ "$top" = "{" ] || { echo unbalanced; return; }; stack=${stack#?} ;;
		esac
	done
	[ -z "$stack" ] && echo balanced || echo unbalanced
}
for expr in "(a[b]{c})" "([)]" "{{}}" "(()" "" "a(b)c[d]" "]["; do
	printf '%-12s -> %s\n' "$expr" "$(check "$expr")"
done

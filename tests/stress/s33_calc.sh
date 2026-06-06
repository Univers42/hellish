#!/bin/sh
# Recursive-descent evaluator for + - * / and parentheses. POS walks EXPR and
# each parse_* leaves its result in the global VAL; the per-call accumulators
# are `local`, so recursion through parentheses stays correct.
EXPR=""; POS=0; VAL=0
cur() { printf '%s' "$EXPR" | cut -c$((POS + 1)); }
parse_number() {
	local num ch
	num=""
	while :; do
		ch=$(cur)
		case "$ch" in
		[0-9]) num="$num$ch"; POS=$((POS + 1)) ;;
		*) break ;;
		esac
	done
	VAL=$num
}
parse_factor() {
	local ch
	ch=$(cur)
	if [ "$ch" = "(" ]; then
		POS=$((POS + 1)); parse_expr; POS=$((POS + 1))
	else
		parse_number
	fi
}
parse_term() {
	local acc ch
	parse_factor; acc=$VAL
	while :; do
		ch=$(cur)
		case "$ch" in
		"*") POS=$((POS + 1)); parse_factor; acc=$((acc * VAL)) ;;
		/) POS=$((POS + 1)); parse_factor; acc=$((acc / VAL)) ;;
		*) break ;;
		esac
	done
	VAL=$acc
}
parse_expr() {
	local acc ch
	parse_term; acc=$VAL
	while :; do
		ch=$(cur)
		case "$ch" in
		+) POS=$((POS + 1)); parse_term; acc=$((acc + VAL)) ;;
		-) POS=$((POS + 1)); parse_term; acc=$((acc - VAL)) ;;
		*) break ;;
		esac
	done
	VAL=$acc
}
evaluate() { EXPR="$1"; POS=0; parse_expr; printf '%s = %s\n' "$1" "$VAL"; }
evaluate "2+3*4"
evaluate "(2+3)*4"
evaluate "100-50/5+2"
evaluate "((1+2)*(3+4))"
evaluate "2*3+4*5-6"
evaluate "1000/10/5"

#!/bin/sh
# Reverse-Polish (postfix) calculator. Stack is a space-separated string;
# pop with ${s##* } (last word) and ${s% *} (drop last word). Stresses
# parameter expansion + $(()) + case.
rpn() {
	stack=""
	for tok in $1; do
		case "$tok" in
		*[!0-9]*)
			b=${stack##* }; stack=${stack% *}
			a=${stack##* }; stack=${stack% *}
			case "$tok" in
			+) r=$((a + b)) ;;
			-) r=$((a - b)) ;;
			x) r=$((a * b)) ;;
			/) r=$((a / b)) ;;
			%) r=$((a % b)) ;;
			esac
			stack="$stack $r"
			;;
		*) stack="$stack $tok" ;;
		esac
	done
	echo "${stack# }"
}
rpn "3 4 + 5 x"
rpn "10 2 / 3 -"
rpn "2 3 x 4 5 x +"
rpn "100 9 % 7 x"

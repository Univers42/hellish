#!/bin/sh
# A tiny stack virtual machine. The program is read from a here-document; the
# stack is a space-separated string (push prepends, so it always has a trailing
# space, which keeps the single-element pop correct).
stack=""
push() { stack="$1 $stack"; }
pop() { top=${stack%% *}; stack=${stack#* }; }
run() {
	while read -r op arg; do
		case "$op" in
		PUSH) push "$arg" ;;
		ADD) pop; a=$top; pop; b=$top; push $((a + b)) ;;
		SUB) pop; a=$top; pop; b=$top; push $((b - a)) ;;
		MUL) pop; a=$top; pop; b=$top; push $((a * b)) ;;
		DUP) pop; push "$top"; push "$top" ;;
		PRINT) pop; echo "$top"; push "$top" ;;
		esac
	done
}
run <<'PROG'
PUSH 5
PUSH 3
ADD
PRINT
PUSH 4
MUL
PRINT
DUP
ADD
PRINT
PROG

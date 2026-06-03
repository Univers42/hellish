#!/bin/sh
# Reverse-Polish-Notation calculator with a string-based stack.
# Exercises: arithmetic ($((..)) all operators), set on positional params,
# case, functions, while loops, parameter expansion (${stack##* } etc.),
# error handling + exit codes, trap, command substitution.
set -u

# stack stored as space-separated string; top is last token.
stack=""

push() { stack="$stack $1"; }
depth() {
	set -- $stack
	echo $#
}
pop() {
	# echoes top, removes it from stack
	set -- $stack
	if [ $# -eq 0 ]; then echo "ERR_UNDERFLOW"; return 1; fi
	eval "top=\${$#}"
	# rebuild without last
	stack=""
	i=1
	while [ $i -lt $# ]; do
		eval "v=\${$i}"
		stack="$stack $v"
		i=$(( i + 1 ))
	done
	echo "$top"
}

apply() {
	op=$1
	b=$(pop) || { echo "underflow on $op" >&2; return 2; }
	a=$(pop) || { echo "underflow on $op" >&2; return 2; }
	case $op in
		+) push $(( a + b )) ;;
		-) push $(( a - b )) ;;
		x) push $(( a * b )) ;;
		/) if [ "$b" -eq 0 ]; then echo "div by zero" >&2; return 3; fi
		   push $(( a / b )) ;;
		%) if [ "$b" -eq 0 ]; then echo "mod by zero" >&2; return 3; fi
		   push $(( a % b )) ;;
		'**') # integer power
			r=1; e=$b
			while [ "$e" -gt 0 ]; do r=$(( r * a )); e=$(( e - 1 )); done
			push "$r" ;;
		*) echo "bad op $op" >&2; return 4 ;;
	esac
}

eval_rpn() {
	expr=$1
	stack=""
	for tok in $expr; do
		case $tok in
			[0-9]*|-[0-9]*) push "$tok" ;;
			+|-|x|/|%|'**') apply "$tok" || return $? ;;
			*) echo "unknown token: $tok" >&2; return 5 ;;
		esac
	done
	res=$(pop) || return 1
	d=$(depth)
	if [ "$d" -ne 0 ]; then echo "leftover stack: $stack" >&2; return 6; fi
	echo "$res"
}

run_case() {
	label=$1
	expr=$2
	out=$(eval_rpn "$expr"); rc=$?
	printf '%-16s "%s" = %s (rc=%d)\n' "$label" "$expr" "$out" "$rc"
}

echo "=== RPN evaluations ==="
run_case "add"        "3 4 +"
run_case "mul-chain"  "2 3 4 x x"
run_case "mixed"      "10 2 / 3 +"
run_case "sub-neg"    "5 8 -"
run_case "power"      "2 10 **"
run_case "modulo"     "17 5 %"
run_case "complex"    "3 4 + 5 6 + x"
run_case "deep"       "1 2 + 3 + 4 + 5 + 6 +"
run_case "divzero"    "5 0 /"
run_case "underflow"  "3 +"
run_case "leftover"   "1 2 3 +"

echo "=== batch from heredoc, summing results ==="
total=0; ok=0
while read -r expr; do
	[ -z "$expr" ] && continue
	r=$(eval_rpn "$expr") && {
		total=$(( total + r ))
		ok=$(( ok + 1 ))
		printf '  %-12s => %d\n' "$expr" "$r"
	}
done <<'EXPRS'
10 20 +
100 7 %
2 2 2 x x
50 5 /
EXPRS
echo "batch_ok=$ok batch_total=$total"

echo "=== factorial via RPN-ish loop ==="
fact() {
	n=$1; r=1
	while [ "$n" -gt 1 ]; do r=$(( r * n )); n=$(( n - 1 )); done
	echo "$r"
}
for k in 1 5 10 12; do
	printf 'fact(%d)=%s\n' "$k" "$(fact "$k")"
done
echo "done"

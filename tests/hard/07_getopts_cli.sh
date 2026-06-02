#!/bin/sh
# A CLI tool with getopts option parsing, subcommands via case, validation,
# usage text, and an action log written to a temp file then replayed.
# Exercises: getopts (OPTIND/OPTARG/opt errors), positional shift, case,
# functions, set -u, here-docs, command substitution, exit codes.
set -u
log=$(mktemp)
trap 'rm -f "$log"' EXIT

usage() {
	cat <<'USAGE'
usage: tool [-v] [-n N] [-o FILE] CMD [args...]
  CMD: add A B | mul A B | greet NAME | list
USAGE
}

verbose=0
count=1
outfile="-"

# Parse global options with getopts.
OPTIND=1
while getopts "vn:o:h" opt; do
	case $opt in
		v) verbose=$((verbose+1)) ;;
		n) count=$OPTARG ;;
		o) outfile=$OPTARG ;;
		h) usage; exit 0 ;;
		\?) echo "bad option" >&2; exit 2 ;;
		:) echo "missing arg" >&2; exit 2 ;;
	esac
done
shift $((OPTIND - 1))

logit() { printf '%s\n' "$*" >> "$log"; }

echo "opts: verbose=$verbose count=$count outfile=$outfile"
echo "remaining args: $#"

cmd=${1:-list}
[ $# -gt 0 ] && shift

run_one() {
	case $cmd in
		add)
			a=${1:-0}; b=${2:-0}
			logit "add $a + $b = $((a+b))"
			;;
		mul)
			a=${1:-1}; b=${2:-1}
			logit "mul $a * $b = $((a*b))"
			;;
		greet)
			name=${1:-world}
			logit "hello, $name (x$count)"
			;;
		list)
			logit "listing items"
			;;
		*)
			echo "unknown cmd: $cmd" >&2
			return 3
			;;
	esac
	return 0
}

# Run the command 'count' times.
i=0
rc=0
while [ "$i" -lt "$count" ]; do
	run_one "$@" || rc=$?
	i=$((i+1))
done

echo "=== action log ($(wc -l < "$log" | tr -d ' ') lines) ==="
cat "$log"
echo "cmd_rc=$rc"

# Demonstrate getopts re-parse on a synthetic argv inside a function.
parse_sub() {
	OPTIND=1
	local_flags=""
	while getopts "abc:" o; do
		case $o in
			a) local_flags="${local_flags}A" ;;
			b) local_flags="${local_flags}B" ;;
			c) local_flags="${local_flags}C:$OPTARG" ;;
			\?) local_flags="${local_flags}?" ;;
		esac
	done
	shift $((OPTIND-1))
	printf 'flags=[%s] rest=[%s]\n' "$local_flags" "$*"
}

echo "=== sub getopts ==="
parse_sub -a -b -c hello extra1 extra2
parse_sub -ab -c x
parse_sub -z file
parse_sub plain args only
echo "done"

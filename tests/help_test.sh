#!/bin/bash
# The `help` builtin (issue #22).
#
# The check that matters is the first one: EVERY builtin the dispatch table
# knows must have a help entry. Documentation rots by omission -- someone
# adds a builtin, forgets the help line, and `help` quietly becomes a list
# of most of the shell. Deriving the expected set from
# hash_builtins_dispatch.c means the test fails the moment that happens,
# rather than the docs drifting silently.
#
# Usage: bash tests/help_test.sh [path/to/hellish]
set -u
cd "$(dirname "$0")/.." || exit 1
H="${1:-./build/bin/hellish}"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1
fails=0

ok()   { printf 'ok   %s\n' "$1"; }
bad()  { printf 'FAIL %s\n       %s\n' "$1" "${2:-}"; fails=$((fails + 1)); }

# 1. every builtin is documented
missing=""
for b in $(grep -o 'hash_set(h, "[^"]*"' src/builtins/hash_builtins_dispatch.c \
		| sed 's/.*"\(.*\)"/\1/'); do
	"$H" -c "help -s '$b'" >/dev/null 2>&1 || missing="$missing $b"
done
if [ -z "$missing" ]; then
	ok "every builtin in the dispatch table has a help entry"
else
	bad "every builtin in the dispatch table has a help entry" "missing:$missing"
fi

# 2. and every documented builtin really is one (no entries for things that
#    do not exist -- the other direction of the same rot)
ghost=""
for t in $("$H" -c 'help' 2>/dev/null | sed -n 's/^  \([a-z.:[][^ ]*\) .*/\1/p'); do
	case "$t" in
		"for("*|"(("*|'$(('*|'$('*|"[["|"["|redirection|pipeline) continue ;;
		for|while|until|if|case|function) continue ;;
	esac
	grep -q "hash_set(h, \"$t\"" src/builtins/hash_builtins_dispatch.c \
		|| ghost="$ghost $t"
done
if [ -z "$ghost" ]; then
	ok "no help entry names a builtin that does not exist"
else
	bad "no help entry names a builtin that does not exist" "ghosts:$ghost"
fi

# 3. the syntax topics are there -- a shell whose help covers only builtins
#    teaches half a language
for t in for while until if case function redirection pipeline; do
	"$H" -c "help -s $t" >/dev/null 2>&1 \
		&& ok "help knows the syntax topic '$t'" \
		|| bad "help knows the syntax topic '$t'"
done

# 4. statuses and streams
"$H" -c 'help' >/dev/null 2>&1 && ok "bare help exits 0" || bad "bare help exits 0"
"$H" -c 'help cd' >/dev/null 2>&1 && ok "help NAME exits 0" || bad "help NAME exits 0"
"$H" -c 'help nosuchtopic_zz' >/dev/null 2>&1 \
	&& bad "unknown topic exits non-zero" || ok "unknown topic exits non-zero"
out=$("$H" -c 'help nosuchtopic_zz' 2>/dev/null)
[ -z "$out" ] && ok "unknown topic says nothing on stdout" \
	|| bad "unknown topic says nothing on stdout" "$out"
# bash's rule, measured: 0 when at least one topic matched, 1 only when
# none did. Error wording is free, exit codes are not.
"$H" -c 'help cd nosuchtopic_zz' >/dev/null 2>&1 \
	&& ok "a good topic among bad ones exits 0, as bash does" \
	|| bad "a good topic among bad ones exits 0, as bash does"
"$H" -c 'help nosuch1_zz nosuch2_zz' >/dev/null 2>&1 \
	&& bad "all-unknown topics exit 1" || ok "all-unknown topics exit 1"

# 5. -s is the short form
full=$("$H" -c 'help cd' 2>/dev/null | wc -l)
short=$("$H" -c 'help -s cd' 2>/dev/null | wc -l)
[ "$short" -lt "$full" ] && ok "-s prints less than the full entry" \
	|| bad "-s prints less than the full entry" "full=$full short=$short"

# 6. it survives a narrow terminal
COLUMNS=40 "$H" -c 'help' >/dev/null 2>&1 \
	&& ok "the listing renders at 40 columns" \
	|| bad "the listing renders at 40 columns"

printf '\n%d check(s) failed\n' "$fails"
[ "$fails" -eq 0 ]

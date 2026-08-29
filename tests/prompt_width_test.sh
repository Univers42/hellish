#!/bin/bash
# Build and run tests/prompt_width_test.c -- the prompt width model, tested
# by linking the object files directly.
#
# A unit test rather than a pty case, and that was a deliberate second
# choice: the width is what the line editor uses to place the cursor, it is
# not printed anywhere, and no shell-level command reveals it. The only
# shell-level observable is where a line wraps on a terminal, and a pty case
# reading that back proved too timing-dependent to gate on -- it passed
# against a binary that still had the bug, which is the one outcome a test
# may never have.
#
# Linking the objects answers exactly, every time. Against the object built
# before the OSC fix, five of eleven cases fail; that is the check that this
# check works.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/tests/prompt_width_test.c"
BIN="${TMPDIR:-/tmp}/hellish_prompt_width_test.$$"

# Two objects, because the width walk and the escape-skipping live in
# separate files -- the norm caps functions per file, so the split is not
# optional and the test has to follow it.
#
# The DEBUG tree is preferred over release: release is built with -flto, so
# its objects carry GCC IR rather than a real symbol table and a plain `cc`
# link cannot see visible_width_cstr at all. Debug objects link directly and
# only need -fsanitize=address adding back.
OBJ=""
EXTRA=""
for d in "$ROOT"/build/obj-debug-*/infrastructure; do
	if [ -e "$d/visible_with_cstr.o" ] && [ -e "$d/visible_skip.o" ]; then
		OBJ="$d/visible_with_cstr.o $d/visible_skip.o"
		EXTRA="-fsanitize=address"
		break
	fi
done
if [ -z "$OBJ" ]; then
	for d in "$ROOT"/build/obj-*/infrastructure; do
		if [ -e "$d/visible_with_cstr.o" ] && [ -e "$d/visible_skip.o" ]; then
			OBJ="$d/visible_with_cstr.o $d/visible_skip.o"
			EXTRA="-flto"
			break
		fi
	done
fi

if [ -z "$OBJ" ]; then
	printf '  SKIP prompt_width_test: no built objects yet (run make first)\n'
	exit 0
fi

trap 'rm -f "$BIN"' EXIT

# shellcheck disable=SC2086  -- OBJ is deliberately two paths, EXTRA a flag
if ! cc -Wall -Wextra -Werror $EXTRA -o "$BIN" "$SRC" $OBJ 2>&1; then
	printf '  FAIL prompt_width_test: did not build\n'
	exit 1
fi

printf '  using %s\n' "$(echo "$OBJ" | sed "s|$ROOT/||g")"
"$BIN"

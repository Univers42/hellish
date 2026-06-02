#!/bin/sh
# File-descriptor redirection: 2>&1 merging and discarding via /dev/null.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# Merge stderr into stdout, capture combined into a file, then print it.
{
	echo "to stdout"
	echo "to stderr" >&2
} >"$work/combined" 2>&1
echo "-- combined file --"
sort "$work/combined"

# stdout to file, stderr discarded: list two known files, error on a missing
# path is sent to /dev/null. We cd into the dir first so output is the bare
# names (no random tmp path) and stays deterministic.
echo alpha >"$work/a"
echo beta >"$work/b"
( cd "$work" && ls a b nosuchfile ) >"$work/out" 2>/dev/null
echo "-- ls captured (errors suppressed) --"
cat "$work/out"

# order of redirections: 2>&1 before > sends stderr to original stdout (terminal),
# while > after still redirects stdout. We test the common combined form only.
echo "value=$(echo hi 2>/dev/null)"

#!/usr/bin/env bash
# ============================================================================
# gen_fixtures.sh -- (re)create the hostile-filename glob fixtures that used
# to be tracked in git: names containing CR, tab, '*', '?', '[', ']', '^',
# '!' and backslashes, used by the globbing/wildcards categories to exercise
# the glob engine against worst-case directory entries.
#
# NTFS cannot represent most of these names, so Git for Windows refuses to
# check them out -- with them tracked, the repo could not even be cloned
# natively on Windows. They are therefore generated at harness startup
# instead of being tracked.
#
# Idempotent, safe from any cwd. On a filesystem that rejects a name (NTFS),
# that fixture is skipped silently -- the Windows skip-list covers the cases
# that would have needed it. Always exits 0.
# ============================================================================
HERE="$(cd "$(dirname "$0")" && pwd)"

mk() {
	[ -e "$HERE/$1" ] || : > "$HERE/$1" 2>/dev/null || true
}

mk $'\r'
mk '&&'
mk '*'

mkdir -p "$HERE/glob-zoo/symbols" 2>/dev/null || true
mk 'glob-zoo/a*b'
mk $'glob-zoo/tab\tfile'
for s in '!' '*' '?' '[' ']' '^' '\*' '\?'; do
	mk "glob-zoo/symbols/$s"
done

exit 0

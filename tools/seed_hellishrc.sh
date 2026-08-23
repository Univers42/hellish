#!/bin/sh
# ============================================================================
# tools/seed_hellishrc.sh -- put hellishrc.example at ~/.hellishrc, once, and
# keep the one block in it that the INSTALLER owns: the PATH entry for the
# directory the binary actually landed in.
#
# Why this is its own file. The seeding used to live inline in
# user-install.sh, which meant it only happened on the no-sudo route. The
# sudo route -- `make my_shell`, install to /usr/bin and chsh -- installed a
# binary and stopped, so a user who took it got a shell with no config at
# all: no EDITOR, no aliases, no PS1, and no hint that a config file was
# ever meant to exist. That is the first line of issue #51. One seeder,
# called by both routes, is the only shape that cannot drift again.
#
# The one rule that matters: an EXISTING ~/.hellishrc is never touched. It
# is the user's file, installers get re-run, and eating an edited config is
# far worse than skipping a template. --path-dir does not break that rule:
# it rewrites ONLY the text between its own markers, the same way
# user-install.sh manages its block in your login shell's rc.
#
#   tools/seed_hellishrc.sh                      seed $HOME/.hellishrc
#   tools/seed_hellishrc.sh --home DIR           seed DIR/.hellishrc
#   tools/seed_hellishrc.sh --example FILE       use FILE as the template
#   tools/seed_hellishrc.sh --path-dir DIR       ...and keep DIR on PATH
#   tools/seed_hellishrc.sh --strip-path         drop the PATH block again
#
# Always exits 0 unless it was asked to do something impossible: a missing
# template is a warning, because an install must not fail over a doc file.
# ============================================================================
set -u

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET_HOME="${HOME:-}"
EXAMPLE="$REPO_ROOT/hellishrc.example"
PATH_DIR=""
STRIP_PATH=0

while [ $# -gt 0 ]; do
	case "$1" in
	--home) TARGET_HOME="${2:-}"; shift 2 ;;
	--example) EXAMPLE="${2:-}"; shift 2 ;;
	--path-dir) PATH_DIR="${2:-}"; shift 2 ;;
	--strip-path) STRIP_PATH=1; shift ;;
	-h|--help) sed -n '3,28p' "$0"; exit 0 ;;
	*) echo "seed_hellishrc: unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$TARGET_HOME" ]; then
	echo "seed_hellishrc: no \$HOME and no --home; nothing to seed" >&2
	exit 2
fi

RC="$TARGET_HOME/.hellishrc"

PATH_BEGIN='# >>> hellish path >>>'
PATH_END='# <<< hellish path <<<'

# ── seeding ────────────────────────────────────────────────────────────────
seed_rc() {
	if [ -e "$RC" ]; then
		printf '  \033[1;34mi\033[0m  %s already exists — left untouched\n' "$RC"
		return 0
	fi

	if [ ! -f "$EXAMPLE" ]; then
		printf '  \033[1;33m!\033[0m  %s not found — skipping %s\n' \
			"$EXAMPLE" "$RC" >&2
		return 0
	fi

	# Copy to a temp name in the same directory and rename, so an interrupted
	# copy can never leave a half-written rc that the next shell then sources.
	cp "$EXAMPLE" "$RC.new" || return 1
	mv -f "$RC.new" "$RC" || return 1
	printf '  \033[1;32m✓\033[0m  seeded %s from %s\n' "$RC" "$(basename "$EXAMPLE")"
}

# ── the managed PATH block ─────────────────────────────────────────────────
# Delete an existing block from stdin. A half-removed block (someone deleted
# one marker by hand) degrades to "leave it alone" rather than eating the
# rest of the file.
strip_path_block() {
	awk -v b="$PATH_BEGIN" -v e="$PATH_END" '
		$0 == e { skip = 0; next }
		$0 == b { skip = 1 }
		skip    { next }
		        { print }
	'
}

# Drop trailing blank lines. We insert a blank line ahead of the block, so
# without this an install/uninstall/install cycle would slowly grow a gap at
# the end of the file. Interior blank lines are preserved.
trim_trailing_blanks() {
	awk 'BEGIN { n = 0 }
	     /^[[:space:]]*$/ { n++; next }
	     { while (n-- > 0) print ""; n = 0; print }'
}

write_path_block() {
	_dir="$1"
	[ -e "$RC" ] || : > "$RC"
	strip_path_block < "$RC" | trim_trailing_blanks > "$RC.new" || return 1

	cat >> "$RC.new" <<EOF

$PATH_BEGIN
# Added by hellish's installer. Everything between the markers is
# regenerated on re-install and removed by \`make user-uninstall\` — put your
# own edits outside them.
#
# Why this is here at all. \`make user-install\` puts the binary somewhere
# your login chain may well NOT have on PATH. Debian/Ubuntu's stock
# ~/.profile adds ~/.local/bin only when that directory already exists at
# login — and on a first install this is the run that created it. A
# user-installed hellish is also exec'd from an interactive rc, so it is not
# a login shell and never reads /etc/profile or ~/.profile itself.
#
# The shell still STARTS without this (the rc hook execs an absolute path),
# which is exactly why the gap was easy to miss: what broke was the NAME.
# \`hellish\`, \`hellish update\`, \`command -v hellish\` and every tool that
# looks the shell up by name answered "command not found" on a freshly
# installed machine. See tests/user_install_path_test.py.
#
# The case guard is load-bearing: this file is re-sourced by every nested
# interactive hellish, and an unguarded prepend stacks a fresh copy each
# time until PATH is mostly duplicates.
case ":\$PATH:" in
	*":$_dir:"*) ;;
	*) PATH="$_dir:\$PATH"; export PATH ;;
esac
$PATH_END
EOF

	# Refuse to leave behind an rc that will not parse: this file is sourced
	# by every interactive hellish, and this script is the one that wrote it.
	if ! sh -n "$RC.new" 2>/dev/null; then
		rm -f "$RC.new"
		printf '  \033[1;33m!\033[0m  generated %s would not parse — left untouched\n' \
			"$RC" >&2
		return 1
	fi
	mv -f "$RC.new" "$RC" || return 1
	printf '  \033[1;32m✓\033[0m  %s puts %s on PATH\n' "$RC" "$_dir"
}

remove_path_block() {
	[ -f "$RC" ] || return 0
	grep -qxF "$PATH_BEGIN" "$RC" 2>/dev/null || return 0
	strip_path_block < "$RC" | trim_trailing_blanks > "$RC.new" || return 1
	mv -f "$RC.new" "$RC" || return 1
	printf '  \033[1;32m✓\033[0m  removed the PATH block from %s\n' "$RC"
}

if [ "$STRIP_PATH" = "1" ]; then
	remove_path_block || exit 1
	exit 0
fi

seed_rc || exit 1
if [ -n "$PATH_DIR" ]; then
	write_path_block "$PATH_DIR" || exit 1
fi
exit 0

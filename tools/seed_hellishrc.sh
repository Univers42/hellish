#!/bin/sh
# ============================================================================
# tools/seed_hellishrc.sh -- put hellishrc.example at ~/.hellishrc, once.
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
# far worse than skipping a template.
#
#   tools/seed_hellishrc.sh                      seed $HOME/.hellishrc
#   tools/seed_hellishrc.sh --home DIR           seed DIR/.hellishrc
#   tools/seed_hellishrc.sh --example FILE       use FILE as the template
#
# Always exits 0 unless it was asked to do something impossible: a missing
# template is a warning, because an install must not fail over a doc file.
# ============================================================================
set -u

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET_HOME="${HOME:-}"
EXAMPLE="$REPO_ROOT/hellishrc.example"

while [ $# -gt 0 ]; do
	case "$1" in
	--home) TARGET_HOME="${2:-}"; shift 2 ;;
	--example) EXAMPLE="${2:-}"; shift 2 ;;
	-h|--help) sed -n '3,20p' "$0"; exit 0 ;;
	*) echo "seed_hellishrc: unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$TARGET_HOME" ]; then
	echo "seed_hellishrc: no \$HOME and no --home; nothing to seed" >&2
	exit 2
fi

RC="$TARGET_HOME/.hellishrc"

if [ -e "$RC" ]; then
	printf '  \033[1;34mi\033[0m  %s already exists — left untouched\n' "$RC"
	exit 0
fi

if [ ! -f "$EXAMPLE" ]; then
	printf '  \033[1;33m!\033[0m  %s not found — skipping %s\n' \
		"$EXAMPLE" "$RC" >&2
	exit 0
fi

# Copy to a temp name in the same directory and rename, so an interrupted
# copy can never leave a half-written rc that the next shell then sources.
cp "$EXAMPLE" "$RC.new" || exit 1
mv -f "$RC.new" "$RC" || exit 1
printf '  \033[1;32m✓\033[0m  seeded %s from %s\n' "$RC" "$(basename "$EXAMPLE")"

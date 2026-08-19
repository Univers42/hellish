#!/bin/bash
# Guard the release/update configuration that no runtime test can reach.
#
# The update builtin, install.sh and the npm installer all bake in a
# GitHub slug. It was "Univers42/42sh" -- a repository that does not
# exist -- so every one of them 404'd, and `update` reported the 404 as
# "could not reach GitHub (offline?)". Nothing failed loudly, and no test
# could have caught it: the suite never runs the update path, and the
# binary is perfectly healthy.
#
# So this checks the configuration itself, offline:
#   1. the slug in version.h matches the repository we actually push to
#   2. every distribution channel agrees with version.h
#   3. the npm package version matches HELLISH_VERSION
#
# Usage: bash tests/update_config_check.sh
set -u
cd "$(dirname "$0")/.." || exit 1
fails=0

check() {
	if [ "$2" = "$3" ]; then
		printf 'ok   %s\n' "$1"
	else
		printf 'FAIL %s\n       want %s\n       got  %s\n' "$1" "$3" "$2"
		fails=$((fails + 1))
	fi
}

# 1. version.h vs the git remote --------------------------------------------
slug=$(sed -n 's/^# *define HELLISH_REPO "\([^"]*\)".*/\1/p' incs/version.h)
remote=$(git config --get remote.origin.url 2>/dev/null \
	| sed -e 's#^git@github.com:##' -e 's#^https://github.com/##' -e 's#\.git$##')
check "version.h HELLISH_REPO matches the git remote" "$slug" "$remote"

# 2. every channel points at the same repository -----------------------------
check "install.sh REPO" \
	"$(sed -n 's/^REPO="\([^"]*\)".*/\1/p' install.sh)" "$slug"
check "npm/scripts/install.js REPO" \
	"$(sed -n "s/^const REPO = '\([^']*\)'.*/\1/p" npm/scripts/install.js)" "$slug"

for f in Dockerfile npm/package.json npm/bin/hellish.js npm/README.md \
	install.sh wiki/product.md; do
	bad=$(grep -c "github.com/Univers42/42sh" "$f" 2>/dev/null)
	check "$f has no stale repo URL" "${bad:-0}" "0"
done

# 3. the npm package version tracks version.h --------------------------------
v=$(sed -n 's/^# *define HELLISH_VERSION "\([^"]*\)".*/\1/p' incs/version.h)
check "npm/package.json version" \
	"$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' npm/package.json | head -1)" "$v"

printf '\n%d check(s) failed\n' "$fails"
[ "$fails" -eq 0 ]

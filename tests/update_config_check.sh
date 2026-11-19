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
#   4. the release publishes BOTH binary names (polish- and hellish-), each
#      with a .sha256: every <= 3.3.0 install updates from the hellish one
#
# The macros are read as POLISH_* or HELLISH_*: the rename (3.4.0) moves
# version.h to the new spelling, and this check must not care which.
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
macro() {
	sed -n -e "s/^# *define POLISH_$1 \"\([^\"]*\)\".*/\1/p" \
		-e "s/^# *define HELLISH_$1 \"\([^\"]*\)\".*/\1/p" incs/version.h \
		| head -1
}
slug=$(macro REPO)
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

# The pre-rename slug only works through GitHub's redirect, which vanishes
# the day anyone creates a repo by that name. hellishrc_plugins is a
# different repository and keeps its name.
for f in install.sh npm/package.json npm/README.md npm/scripts/install.js \
	npm/bin/hellish.js Dockerfile docker/Dockerfile.release mkdocs.yml \
	tools/fetch_release.sh; do
	bad=$(grep -cE 'github\.com/Univers42/hellish([^a-z_]|$)' "$f" 2>/dev/null)
	check "$f has no pre-rename repo URL" "${bad:-0}" "0"
done

# 3. the npm package version tracks version.h --------------------------------
v=$(macro VERSION)
check "npm/package.json version" \
	"$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' npm/package.json | head -1)" "$v"

# 4. both asset names are published ------------------------------------------
for a in polish-linux-x86_64 polish-linux-x86_64.sha256 \
	hellish-linux-x86_64 hellish-linux-x86_64.sha256; do
	n=$(grep -cxE "[[:space:]]+$a" .github/workflows/release.yml)
	check "release.yml publishes $a" "$n" "1"
done

printf '\n%d check(s) failed\n' "$fails"
[ "$fails" -eq 0 ]

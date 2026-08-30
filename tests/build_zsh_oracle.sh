#!/bin/bash
# Build the zsh oracle: zsh 5.9, which tests/zsh_flags_test.py diffs the zsh
# dialect against -- the same arrangement tests/build_oracle.sh sets up for
# bash 5.3.9 and the golden suite.
#
#   ./tests/build_zsh_oracle.sh [prefix]     (default: ~/zsh-5.9)
#
# WHY THIS IS NOT OPTIONAL SCAFFOLDING
# The first version of the zsh flag tests asserted what the flags obviously
# do, and passed. A real zsh then disagreed with four of them -- every time
# by giving a MESSIER answer than the one that looked right:
#   "${(o)arr}" does not sort, ${(k)arr} on an indexed array gives values,
#   ${(q)x} quotes with backslashes, and ${(f)} drops empty fields unquoted.
# None of that is inferable from our own source. Without an oracle the suite
# tests our assumptions against themselves.
#
# Idempotent: exits immediately if the oracle is already built at that prefix.
set -eu

PREFIX="${1:-$HOME/zsh-5.9}"
VERSION=5.9

if [ -x "$PREFIX/bin/zsh" ]; then
	printf "  already built: %s\n" "$("$PREFIX/bin/zsh" --version)" >&2
	exit 0
fi

for tool in curl tar make cc; do
	command -v "$tool" >/dev/null 2>&1 || {
		printf "  missing build dependency: %s\n" "$tool" >&2
		exit 1
	}
done

SRC="$(mktemp -d)"
# The source tree is scratch; the installed prefix is the artefact to keep.
trap 'rm -rf "$SRC"' EXIT

cd "$SRC"
printf "  fetching zsh-%s\n" "$VERSION" >&2
curl -sLO "https://www.zsh.org/pub/zsh-$VERSION.tar.xz"
tar xf "zsh-$VERSION.tar.xz"
cd "zsh-$VERSION"

# --disable-dynamic keeps every module inside the binary, so the oracle needs
# no module path at runtime and cannot silently pick up a system zsh's
# modules -- the same isolation reason the bash oracle installs to its own
# prefix rather than being used from PATH.
printf "  configuring\n" >&2
./configure --prefix="$PREFIX" --disable-dynamic --without-tcsetpgrp \
	>/dev/null 2>&1 || ./configure --prefix="$PREFIX" >/dev/null

printf "  building (this takes a couple of minutes)\n" >&2
make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" >/dev/null
make install >/dev/null

printf "\n  built: %s\n" "$("$PREFIX/bin/zsh" --version)" >&2
printf "  point the suite at it with:  ZSH_ORACLE=%s/bin/zsh\n" "$PREFIX" >&2

#!/bin/bash
# Build the golden oracle: bash 5.3.9, patched exactly as .github/workflows/ci.yml
# builds it, so a local `make test` is diffed against the same specification CI
# uses. See the `oracle` target in the root Makefile for why this exists.
#
#   ./tests/build_oracle.sh [prefix]        (default: ~/bash-5.3.9)
#
# Idempotent: exits immediately if the oracle is already built at that prefix.
set -eu

PREFIX="${1:-$HOME/bash-5.3.9}"
BASE_VERSION=5.3
PATCH_COUNT=9

if [ -x "$PREFIX/bin/bash" ]; then
	printf "  already built: %s\n" "$("$PREFIX/bin/bash" --version | head -1)" >&2
	exit 0
fi

for tool in curl tar patch make cc; do
	command -v "$tool" >/dev/null 2>&1 || {
		printf "  missing build dependency: %s\n" "$tool" >&2
		exit 1
	}
done

SRC="$(mktemp -d)"
# The source tree is scratch; the installed prefix is the artefact worth keeping.
trap 'rm -rf "$SRC"' EXIT

cd "$SRC"
printf "  fetching bash-%s\n" "$BASE_VERSION" >&2
curl -sLO "https://ftp.gnu.org/gnu/bash/bash-$BASE_VERSION.tar.gz"
tar xzf "bash-$BASE_VERSION.tar.gz"
cd "bash-$BASE_VERSION"

# The .9 in 5.3.9 is the patch level: upstream ships fixes as numbered patches
# rather than tarballs, so the pinned version only exists once these are applied.
printf "  applying patches 001..%03d\n" "$PATCH_COUNT" >&2
i=1
while [ "$i" -le "$PATCH_COUNT" ]; do
	p=$(printf 'bash53-%03d' "$i")
	curl -sL "https://ftp.gnu.org/gnu/bash/bash-$BASE_VERSION-patches/$p" \
		| patch -p0 >/dev/null
	i=$((i + 1))
done

printf "  configuring\n" >&2
./configure --prefix="$PREFIX" > configure.log 2>&1
printf "  compiling\n" >&2
make -j"$(nproc 2>/dev/null || echo 4)" > make.log 2>&1
make install > install.log 2>&1

printf "  built: %s\n" "$("$PREFIX/bin/bash" --version | head -1)" >&2

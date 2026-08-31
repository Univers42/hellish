#!/bin/sh
# ============================================================================
# tools/fetch_release.sh -- download a PUBLISHED hellish release binary.
#
# Exists so `make my_shell VERSION=2.7.2` can put a real, old, released binary
# on the machine instead of building HEAD. That is the only way to stand in
# front of a bug that lives in a version you no longer have the sources
# checked out for -- issue #76 is exactly that shape: the failure is in the
# updater that is ALREADY INSTALLED, so reproducing it means installing the
# old one and pressing the button.
#
#   tools/fetch_release.sh 2.7.2 /tmp/hellish-2.7.2
#   tools/fetch_release.sh v2.7.2            # prints to stdout where it landed
#
# Verifies the published sha256 before handing the file back. A release binary
# is about to become the caller's LOGIN SHELL; downloading one over the
# network and installing it unverified is not something this repo should make
# convenient.
# ============================================================================
set -eu

PROG="$(basename "$0")"
REPO="${HELLISH_REPO:-Univers42/hellish}"

red() { printf '\033[1;31m%s\033[0m\n' "$*" >&2; }
grn() { printf '  \033[1;32m✓\033[0m %s\n' "$*" >&2; }
inf() { printf '  \033[1;36m▸\033[0m %s\n' "$*" >&2; }
die() { red "$PROG: $*"; exit 1; }

[ $# -ge 1 ] || die "usage: $PROG <version> [output-path]"

# Accept 2.7.2 and v2.7.2 alike; the tag carries the v, the asset does not.
VER="${1#v}"
OUT="${2:-}"

# The asset name is uname -m, NOT docker's arch spelling -- it has to match
# what the shell's own updater asks for (update_asset_name()), or a machine
# installed this way could never update itself.
ARCH="$(uname -m)"
case "$ARCH" in
x86_64|aarch64) ;;
*) die "no release binary is published for $ARCH" ;;
esac
ASSET="hellish-linux-$ARCH"
BASE="https://github.com/$REPO/releases/download/v$VER"

[ -n "$OUT" ] || OUT="$(mktemp -t "hellish-$VER.XXXXXX")"

command -v curl >/dev/null 2>&1 || die "curl is required"

inf "fetching v$VER ($ASSET)"
curl -fsSL --proto '=https' --max-time 120 -o "$OUT" "$BASE/$ASSET" \
	|| die "could not download $BASE/$ASSET
  is v$VER a real release with a Linux binary? try: gh release list"

# The checksum is published beside the asset. Missing is a weaker release, not
# a corrupt one, so it downgrades to a warning rather than refusing -- but a
# checksum that is present and WRONG stops everything.
_sum="$OUT.sha256"
if curl -fsSL --proto '=https' --max-time 30 -o "$_sum" \
		"$BASE/$ASSET.sha256" 2>/dev/null; then
	_want="$(cut -d' ' -f1 < "$_sum")"
	_got="$(sha256sum "$OUT" | cut -d' ' -f1)"
	rm -f "$_sum"
	[ "$_want" = "$_got" ] || { rm -f "$OUT"; die "CHECKSUM MISMATCH for v$VER
  published $_want
  received  $_got
  refusing to install it."; }
	grn "sha256 verified"
else
	rm -f "$_sum"
	printf '  \033[1;33m!\033[0m v%s publishes no checksum — cannot verify\n' \
		"$VER" >&2
fi

chmod 755 "$OUT"

# Prove it runs HERE before anything installs it as a login shell. Old
# releases were dynamically linked (see .github/workflows/release.yml), so a
# binary that needs a libreadline this machine does not have fails right here
# instead of at the caller's next login.
# NOT `if ! cmd; then _st=$?`: `!` inverts the status, so $? inside the branch
# is 0/1 and never the 42 we asked for -- the check would fail against a
# binary that ran perfectly. Same shape as register_shell.sh's smoke_test.
_st=0
HELLISH_NO_BANNER=1 HELLISH_NO_ANIM=1 HELLISH_NO_UPDATE_CHECK=1 \
	"$OUT" -c 'exit 42' >/dev/null 2>&1 || _st=$?
if [ "$_st" != "42" ]; then
	"$OUT" -c 'exit 42' 2>&1 | head -3 >&2
	rm -f "$OUT"
	die "the downloaded v$VER binary does not run on this machine
  (exit $_st, wanted 42) -- releases before v2.7.7 were dynamically linked
  and need libreadline.so.8 plus a recent glibc."
fi
grn "v$VER runs here: $OUT"

printf '%s\n' "$OUT"

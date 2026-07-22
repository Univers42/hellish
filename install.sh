#!/bin/sh
# hellish installer — fetch the latest prebuilt binary from GitHub Releases.
#
#   curl -fsSL https://raw.githubusercontent.com/Univers42/42sh/main/install.sh | sh
#
# Honours $PREFIX (default: /usr/local/bin, or ~/.local/bin without write access).
set -eu

REPO="Univers42/42sh"
ASSET="hellish-linux-x86_64"
OS="$(uname -s)"
ARCH="$(uname -m)"

if [ "$OS" != "Linux" ] || [ "$ARCH" != "x86_64" ]; then
	echo "hellish: no prebuilt binary for $OS/$ARCH yet — build from source:" >&2
	echo "  git clone https://github.com/$REPO && cd 42sh && make OPT=1 all" >&2
	exit 1
fi

URL="https://github.com/$REPO/releases/latest/download/$ASSET"
TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT

echo "hellish: downloading the latest release…"
if command -v curl >/dev/null 2>&1; then
	curl -fSL "$URL" -o "$TMP"
else
	wget -O "$TMP" "$URL"
fi
chmod +x "$TMP"

SUDO=""
if [ -n "${PREFIX:-}" ]; then
	DEST="$PREFIX/hellish"
elif [ -w /usr/local/bin ]; then
	DEST="/usr/local/bin/hellish"
elif command -v sudo >/dev/null 2>&1; then
	DEST="/usr/local/bin/hellish"
	SUDO="sudo"
else
	mkdir -p "$HOME/.local/bin"
	DEST="$HOME/.local/bin/hellish"
fi

$SUDO mkdir -p "$(dirname "$DEST")"
$SUDO mv "$TMP" "$DEST"
trap - EXIT

echo "hellish: installed → $DEST"
echo "make it a login shell:   echo $DEST | sudo tee -a /etc/shells && chsh -s $DEST"
echo "                         (chsh only affects sessions you start AFTER it —"
echo "                          your current terminal stays on the old shell)"
echo "or just run:             $DEST"

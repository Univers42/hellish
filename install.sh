#!/bin/sh
# hellish installer — fetch the latest prebuilt binary from GitHub Releases.
#
#   curl -fsSL https://raw.githubusercontent.com/Univers42/42sh/main/install.sh | sh
#
# Modes (pass after `sh -s --` when piping, e.g. `... | sh -s -- --with-ai`):
#   --shell-only   just the shell (default)
#   --with-ai      the shell + an AI config in ~/.hellishrc; LLM runs in docker
#   --docker-full  install nothing on the host; run the whole stack in docker
#
# Honours $PREFIX (default: /usr/local/bin, or ~/.local/bin without write access).
set -eu

REPO="Univers42/42sh"
ASSET="hellish-linux-x86_64"
MODE="${1:-${HELLISH_INSTALL_MODE:---shell-only}}"

# --- install the prebuilt binary (host) -----------------------------------
install_binary() {
	OS="$(uname -s)"
	ARCH="$(uname -m)"
	if [ "$OS" != "Linux" ] || [ "$ARCH" != "x86_64" ]; then
		echo "hellish: no prebuilt binary for $OS/$ARCH — build from source:" >&2
		echo "  git clone https://github.com/$REPO && cd 42sh && make OPT=1 all" >&2
		exit 1
	fi
	TMP="$(mktemp)"
	trap 'rm -f "$TMP"' EXIT
	echo "hellish: downloading the latest release…"
	if command -v curl >/dev/null 2>&1; then
		curl -fSL "https://github.com/$REPO/releases/latest/download/$ASSET" -o "$TMP"
	else
		wget -O "$TMP" "https://github.com/$REPO/releases/latest/download/$ASSET"
	fi
	chmod +x "$TMP"
	SUDO=""
	if [ -n "${PREFIX:-}" ]; then DEST="$PREFIX/hellish"
	elif [ -w /usr/local/bin ]; then DEST="/usr/local/bin/hellish"
	elif command -v sudo >/dev/null 2>&1; then DEST="/usr/local/bin/hellish"; SUDO="sudo"
	else mkdir -p "$HOME/.local/bin"; DEST="$HOME/.local/bin/hellish"; fi
	$SUDO mkdir -p "$(dirname "$DEST")"
	$SUDO mv "$TMP" "$DEST"
	trap - EXIT
	echo "hellish: installed → $DEST"
}

# --- write the AI config into ~/.hellishrc (idempotent) --------------------
write_ai_config() {
	rc="$HOME/.hellishrc"
	if [ -f "$rc" ] && grep -q 'hellish AI (installer)' "$rc" 2>/dev/null; then
		echo "hellish: AI config already present in $rc"
		return 0
	fi
	{
		echo "# hellish AI (installer)"
		echo "set -o ai"
		echo "export HELLISH_AI_HOST=127.0.0.1"
		echo "export HELLISH_AI_PORT=8080"
	} >> "$rc"
	echo "hellish: AI enabled in $rc (set -o ai, server 127.0.0.1:8080)"
}

# --- print how to bring up the dockerised llama.cpp server -----------------
print_llm_steps() {
	echo ""
	echo "Start the LLM (auto-detects GPU, else CPU) in docker:"
	echo "  git clone https://github.com/$REPO && cd 42sh"
	echo "  make ai-pull MODEL=<url-to-a-.gguf>   # one-time: download a model"
	echo "  make ai-up                            # start llama-server on :8080"
	echo "Then run hellish; the prompt shows AI pro-tips and Ctrl-X Ctrl-A"
	echo "completes the current line. Turn AI off any time with: set +o ai"
}

# --- full docker path: nothing installed on the host ----------------------
docker_full() {
	if ! command -v docker >/dev/null 2>&1; then
		echo "hellish: docker not found — install docker first, then re-run." >&2
		exit 1
	fi
	echo "hellish: cloning the repo and bringing up the docker stack…"
	[ -d 42sh ] || git clone "https://github.com/$REPO"
	cd 42sh
	make ai-up
	echo ""
	echo "LLM is up. Drop into a containerised hellish that talks to it:"
	echo "  make ai-shell"
}

case "$MODE" in
	--docker-full)
		docker_full ;;
	--with-ai)
		install_binary
		write_ai_config
		print_llm_steps ;;
	--shell-only|*)
		install_binary
		echo "make it a login shell:  echo \$DEST | sudo tee -a /etc/shells && chsh -s \$DEST"
		echo "AI mode? re-run with:    ... | sh -s -- --with-ai" ;;
esac

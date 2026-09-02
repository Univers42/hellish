#!/bin/sh
# ============================================================================
# hellish installer -- one command, both privilege worlds.
#
#   curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
#
# What it does, in order:
#   1. downloads the latest static release binary and verifies its sha256;
#   2. detects whether you can use sudo, and routes to the right install:
#        with sudo    -> /usr/bin/hellish + /etc/shells + chsh
#                        (exactly `make my_shell`, via tools/register_shell.sh)
#        without sudo -> ~/.local/bin/hellish + an exec hook in your rc
#                        (exactly `make user-install`, via user-install.sh)
#      Both drivers ship in a small "install bundle" published beside the
#      binary, so this script REUSES the proven scripts instead of
#      reimplementing them -- preflight, smoke-test, never-clobber seeding,
#      uninstall markers and all.
#   3. offers the hellish plugin framework (github.com/Univers42/hellishrc_plugins)
#      and lets you pick plugins -- interactively when a terminal is there
#      (questions read /dev/tty, so `curl | sh` still gets to ask), silently
#      when there is not.
#
# Flags (after `sh -s --` when piped):
#   --user | --system        force the install mode instead of detecting
#   --yes                    assume the default answer to every question
#   --plugins=all|none|LIST  plugin selection without questions
#                            (LIST is space-separated: --plugins="git jump z")
#   --version vX.Y.Z         install that release instead of the latest
#   --no-login-shell         system mode: install the binary, skip chsh
#   --prefix DIR             user mode: install under DIR (default ~/.local)
#   --zshrc | --no-zshrc     load your ~/.zshrc inside hellish (asked when
#                            your login shell is zsh and the file exists)
#   --uninstall              undo a user-mode install (system: make my-shell-uninstall)
#
# Environment (mainly for tests -- docker/Dockerfile.installer drives these):
#   HELLISH_INSTALL_MODE     user|system         same as --user/--system
#   HELLISH_INSTALL_PLUGINS  all|none|LIST       same as --plugins
#   HELLISH_IMPORT_ZSHRC     yes|no              same as --zshrc/--no-zshrc
#   HELLISH_RELEASE_BASE     base URL for release downloads
#                            (default https://github.com/Univers42/hellish/releases)
#   HELLISH_RAW_BASE         base URL for raw-file fallback
#   HELLISH_PLUGINS_SRC      git URL, tarball URL or local dir for the framework
#
# From a source checkout this script uses the local files and, if present, the
# locally built binary -- so `sh install.sh` in the repo works offline.
# ============================================================================
set -eu

REPO="Univers42/hellish"
ASSET="hellish-linux-x86_64"
BUNDLE="hellish-install-bundle.tar.gz"
RELEASE_BASE="${HELLISH_RELEASE_BASE:-https://github.com/$REPO/releases}"
RAW_BASE="${HELLISH_RAW_BASE:-https://raw.githubusercontent.com/$REPO/main}"
PLUGINS_SRC="${HELLISH_PLUGINS_SRC:-https://github.com/Univers42/hellishrc_plugins}"

MODE="${HELLISH_INSTALL_MODE:-}"
PLUGINS="${HELLISH_INSTALL_PLUGINS:-}"
ZSHRC="${HELLISH_IMPORT_ZSHRC:-}"
ASSUME_YES=0
VERSION=""
LOGIN_SHELL=1
UPREFIX="${PREFIX:-$HOME/.local}"
ACTION="install"

say()  { printf '\033[1;36mhellish:\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mhellish:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mhellish:\033[0m %s\n' "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
	case "$1" in
	--user)   MODE="user" ;;
	--system) MODE="system" ;;
	--yes|-y) ASSUME_YES=1 ;;
	--plugins=*) PLUGINS="${1#--plugins=}" ;;
	--plugins)   shift; [ $# -gt 0 ] || die "--plugins needs a value"; PLUGINS="$1" ;;
	--version)   shift; [ $# -gt 0 ] || die "--version needs a tag"; VERSION="$1" ;;
	--no-login-shell) LOGIN_SHELL=0 ;;
	--prefix) shift; [ $# -gt 0 ] || die "--prefix needs a directory"; UPREFIX="$1" ;;
	--zshrc) ZSHRC="yes" ;;
	--no-zshrc) ZSHRC="no" ;;
	--uninstall) ACTION="uninstall" ;;
	-h|--help) sed -n '2,48p' "$0" 2>/dev/null | sed 's/^# \{0,1\}//'; exit 0 ;;
	*) die "unknown argument '$1' (try --help)" ;;
	esac
	shift
done

# ── can we ask questions? ───────────────────────────────────────────────────
# Under `curl | sh`, stdin is the pipe -- but /dev/tty is still the terminal,
# so questions remain possible. No tty (CI, docker build) means every answer
# is the default and the plugin step is skipped unless flags said otherwise.
# The probe runs in a SUBSHELL on purpose: a failed redirection on a special
# builtin like `:` is fatal to a non-interactive POSIX shell, even inside an
# `if` -- the subshell dies in the parent's place.
INTERACTIVE=0
if [ "$ASSUME_YES" = "0" ] && ( exec </dev/tty ) 2>/dev/null; then
	INTERACTIVE=1
fi

# ask "question" "default(y|n)" -> 0 for yes
ask() {
	if [ "$INTERACTIVE" = "0" ]; then
		[ "$2" = "y" ]
		return
	fi
	if [ "$2" = "y" ]; then _p="[Y/n]"; else _p="[y/N]"; fi
	printf '\033[1;36m?\033[0m %s %s ' "$1" "$_p" >/dev/tty
	IFS= read -r _a </dev/tty || _a=""
	case "$_a" in
	[yY]*) return 0 ;;
	[nN]*) return 1 ;;
	*) [ "$2" = "y" ] ;;
	esac
}

fetch() { # fetch URL DEST -> 0/1
	if command -v curl >/dev/null 2>&1; then
		curl -fsSL "$1" -o "$2"
	elif command -v wget >/dev/null 2>&1; then
		wget -q -O "$2" "$1"
	else
		die "neither curl nor wget is available"
	fi
}

# ── where do the pieces come from? ──────────────────────────────────────────
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK=""
cleanup() { [ -n "$WORK" ] && rm -rf "$WORK"; }
trap cleanup EXIT INT TERM

if [ -f "$HERE/Makefile" ] && [ -x "$HERE/tools/register_shell.sh" ]; then
	# A source checkout: the drivers are right here, and a built binary wins
	# over a download.
	SRC_ROOT="$HERE"
	BIN=""
	[ -x "$HERE/build/bin/hellish" ] && BIN="$HERE/build/bin/hellish"
	[ -z "$BIN" ] && [ -x "$HERE/dist/$ASSET" ] && BIN="$HERE/dist/$ASSET"
else
	SRC_ROOT=""
	BIN=""
fi

WORK="$(mktemp -d)"

if [ -z "$BIN" ]; then
	OS="$(uname -s)"; ARCH="$(uname -m)"
	if [ "$OS" != "Linux" ] || [ "$ARCH" != "x86_64" ]; then
		warn "no prebuilt binary for $OS/$ARCH yet -- build from source:"
		warn "  git -c http.version=HTTP/1.1 clone --recursive https://github.com/$REPO"
		warn "  cd hellish && make OPT=1 all"
		exit 1
	fi
	if [ -n "$VERSION" ]; then DL="$RELEASE_BASE/download/$VERSION"
	else DL="$RELEASE_BASE/latest/download"; fi
	say "downloading ${VERSION:-the latest release}..."
	fetch "$DL/$ASSET" "$WORK/hellish" || die "download failed: $DL/$ASSET"
	if fetch "$DL/$ASSET.sha256" "$WORK/hellish.sha256" 2>/dev/null \
		&& command -v sha256sum >/dev/null 2>&1; then
		( cd "$WORK" && sed "s/ .*/  hellish/" hellish.sha256 \
			| sha256sum -c - >/dev/null 2>&1 ) \
			|| die "sha256 verification FAILED -- refusing to install"
		say "sha256 verified"
	else
		warn "could not verify the checksum (missing .sha256 or sha256sum)"
	fi
	chmod +x "$WORK/hellish"
	BIN="$WORK/hellish"
fi

if [ -z "$SRC_ROOT" ]; then
	# The drivers travel as a bundle beside the binary. A release too old to
	# have one falls back to the scripts on main -- they are drivers of a
	# binary, not part of it, so the newest ones are the right ones anyway.
	if [ -n "$VERSION" ]; then DL="$RELEASE_BASE/download/$VERSION"
	else DL="$RELEASE_BASE/latest/download"; fi
	if fetch "$DL/$BUNDLE" "$WORK/bundle.tgz" 2>/dev/null; then
		( cd "$WORK" && tar -xzf bundle.tgz )
	else
		say "no install bundle on this release -- fetching the scripts from main"
		mkdir -p "$WORK/tools" "$WORK/share"
		for f in user-install.sh tools/register_shell.sh \
			tools/seed_hellishrc.sh hellishrc.example; do
			fetch "$RAW_BASE/$f" "$WORK/$f" || die "could not fetch $f"
		done
		chmod +x "$WORK/user-install.sh" "$WORK"/tools/*.sh
	fi
	SRC_ROOT="$WORK"
fi

# ── uninstall (user mode; system mode has richer tooling in the repo) ───────
if [ "$ACTION" = "uninstall" ]; then
	sh "$SRC_ROOT/user-install.sh" --uninstall
	say "for a system install, run from a checkout: make my-shell-uninstall"
	exit 0
fi

# ── which world are we in? ──────────────────────────────────────────────────
if [ -z "$MODE" ]; then
	if [ "$(id -u)" = "0" ]; then
		MODE="system"
	elif command -v sudo >/dev/null 2>&1 && sudo -n true 2>/dev/null; then
		MODE="system"
	elif command -v sudo >/dev/null 2>&1 && [ "$INTERACTIVE" = "1" ] \
		&& ask "You are not root -- do you have sudo rights on this machine?" n; then
		MODE="system"
	else
		MODE="user"
	fi
fi
say "install mode: $MODE$( [ "$MODE" = "user" ] && printf ' (no sudo needed -- the 42-machine path)' )"

# ── install ─────────────────────────────────────────────────────────────────
if [ "$MODE" = "system" ]; then
	# Changing a login shell is never done silently: it happens when a human
	# answered yes, or when --yes said to take the defaults. A tty-less run
	# with no flags gets the binary and keeps its shell.
	if [ "$LOGIN_SHELL" = "1" ] && [ "$INTERACTIVE" = "0" ] \
		&& [ "$ASSUME_YES" = "0" ]; then
		LOGIN_SHELL=0
		say "no terminal to ask on -- installing the binary only (--yes opts into chsh)"
	elif [ "$LOGIN_SHELL" = "1" ] \
		&& ! ask "Register hellish as your login shell (chsh)?" y; then
		LOGIN_SHELL=0
	fi
	( cd "$SRC_ROOT" && sh tools/seed_hellishrc.sh )
	if [ "$LOGIN_SHELL" = "1" ]; then
		( cd "$SRC_ROOT" && sh tools/register_shell.sh \
			--bin "$BIN" --dest /usr/bin/hellish )
	else
		_as_root=""; [ "$(id -u)" = "0" ] || _as_root="sudo"
		$_as_root install -m 0755 "$BIN" /usr/bin/hellish
		say "installed /usr/bin/hellish (login shell untouched; run: hellish)"
	fi
else
	( cd "$SRC_ROOT" && sh user-install.sh --bin "$BIN" --prefix "$UPREFIX" )
fi

# ── your zsh configuration ──────────────────────────────────────────────────
# Every 42 account logs into zsh, and most have a ~/.zshrc they already like.
# hellish reads ~/.hellishrc, not ~/.zshrc, so that config used to stay
# behind -- and pasting it across is what produced issue #112: zsh syntax
# (`precmd() { vcs_info }`) in a bash-dialect file. The bridge is an rc.d
# module with a .zsh EXTENSION: hellish loads it, and everything it sources,
# in the zsh dialect, so the file can simply source ~/.zshrc as written.
#
# Asked only when it applies (login shell is zsh, ~/.zshrc exists). The
# default is yes -- unless the file loads oh-my-zsh itself, whose core needs
# zsh's completion system and would only print what it cannot do.
login_shell_name() {
	_ls=""
	if command -v getent >/dev/null 2>&1; then
		_ls="$(getent passwd "$(id -un)" 2>/dev/null | cut -d: -f7)"
	fi
	[ -n "$_ls" ] || _ls="${SHELL:-/bin/sh}"
	basename "$_ls"
}
case "${XDG_CONFIG_HOME:-}" in
"$HOME"/*) XDG="$XDG_CONFIG_HOME/hellish" ;;
*) XDG="$HOME/.config/hellish" ;;
esac
ZSHRC_MODULE="$XDG/rc.d/90-zshrc.zsh"
if [ "$ZSHRC" != "no" ] && [ -f "$HOME/.zshrc" ] \
	&& { [ "$ZSHRC" = "yes" ] || [ "$(login_shell_name)" = "zsh" ]; }; then
	if [ "$ZSHRC" != "yes" ]; then
		_dflt=y
		grep -q 'oh-my-zsh' "$HOME/.zshrc" 2>/dev/null && _dflt=n
		if [ -e "$ZSHRC_MODULE" ]; then
			ZSHRC="keep"
		elif { [ "$INTERACTIVE" = "1" ] || [ "$ASSUME_YES" = "1" ]; } \
			&& ask "Load your ~/.zshrc inside hellish too (aliases, functions, prompt)?" "$_dflt"; then
			ZSHRC="yes"
		else
			ZSHRC="no"
		fi
	fi
	if [ "$ZSHRC" = "yes" ]; then
		mkdir -p "$XDG/rc.d"
		cat > "$ZSHRC_MODULE" <<'EOF'
# ~/.config/hellish/rc.d/90-zshrc.zsh -- written by hellish's installer.
#
# Loads your ~/.zshrc inside hellish. The .zsh extension is what makes it
# work: hellish reads this file -- and everything it sources -- in the zsh
# dialect, so `precmd() { vcs_info }`, zstyle, PROMPT/RPROMPT and the rest
# of a zsh config run exactly as written. Anything zsh-only that hellish
# cannot do (compinit, zle widgets) is reported once and skipped.
#
# Delete this file to stop loading ~/.zshrc; or replace the `source` with
# the lines you actually want, and keep the .zsh name.
if [ -f "$HOME/.zshrc" ]; then
	# The block the installer appended to ~/.zshrc execs hellish from an
	# interactive zsh. This marker is what tells it not to do so again
	# from inside hellish itself.
	HELLISH_EXECD=1
	export HELLISH_EXECD
	source "$HOME/.zshrc"
fi
EOF
		say "your ~/.zshrc will load inside hellish (zsh dialect): $ZSHRC_MODULE"
	elif [ "$ZSHRC" = "keep" ]; then
		say "keeping $ZSHRC_MODULE"
	fi
fi

# ── plugins ─────────────────────────────────────────────────────────────────
# The framework repo owns the catalog and its own installer; this script only
# asks the coarse questions and hands the answer down, so adding a plugin to
# the catalog never needs a change here.
want_framework=0
if [ -n "$PLUGINS" ] && [ "$PLUGINS" != "none" ]; then
	want_framework=1
elif [ -z "$PLUGINS" ] \
	&& { [ "$INTERACTIVE" = "1" ] || [ "$ASSUME_YES" = "1" ]; } \
	&& ask "Install the hellish plugin framework (git, jump, docker, net + a plugin catalog)?" y; then
	want_framework=1
	if [ "$INTERACTIVE" = "1" ]; then
		printf '\033[1;36m?\033[0m Plugins: [a]ll / [c]hoose one by one / [n]one  [a] ' >/dev/tty
		IFS= read -r _a </dev/tty || _a=""
		case "$_a" in
		[cC]*) PLUGINS="choose" ;;
		[nN]*) PLUGINS="none" ;;
		*)     PLUGINS="all" ;;
		esac
	else
		PLUGINS="all"
	fi
fi

if [ "$want_framework" = "1" ]; then
	say "setting up the plugin framework..."
	FW="$WORK/hellishrc_plugins"
	case "$PLUGINS_SRC" in
	/*)	cp -R "$PLUGINS_SRC" "$FW" ;;
	*.tar.gz|*.tgz)
		fetch "$PLUGINS_SRC" "$WORK/fw.tgz" || die "could not fetch $PLUGINS_SRC"
		mkdir -p "$FW" && tar -xzf "$WORK/fw.tgz" -C "$FW" --strip-components=1 ;;
	*)	fw_ok=0
		# HTTP/1.1 on purpose: git before 2.35 speaking HTTP/2 takes a
		# spurious 401 on the git-upload-pack POST even for public repos
		# (the 42 image ships 2.34.1). Harmless on newer git.
		#   GIT_TERMINAL_PROMPT=0 is the other half of issue #111: on that
		# 401 git ASKED for a username -- and `curl | sh` has a terminal to
		# ask on, so the install sat there until Ctrl-C. A public repo never
		# needs credentials; any request for them is a failure, and failing
		# is what lets the tarball fallback below run. The empty
		# credential.helper keeps a configured helper from popping a GUI.
		if command -v git >/dev/null 2>&1 &&
			GIT_TERMINAL_PROMPT=0 GIT_ASKPASS=/bin/true \
			git -c http.version=HTTP/1.1 -c credential.helper= \
				clone -q --depth 1 "$PLUGINS_SRC" "$FW" 2>/dev/null </dev/null
		then
			fw_ok=1
		fi
		if [ "$fw_ok" = 0 ]; then
			# No git, or the clone failed anyway. The tarball rides plain
			# HTTPS with no git transport, so it survives both cases.
			rm -rf "$FW"
			fetch "${PLUGINS_SRC%.git}/archive/refs/heads/main.tar.gz" \
				"$WORK/fw.tgz" || die "could not fetch the framework"
			mkdir -p "$FW" && tar -xzf "$WORK/fw.tgz" -C "$FW" --strip-components=1
		fi ;;
	esac
	sh "$FW/install.sh" --plugins "${PLUGINS:-all}"
fi

# ── report ──────────────────────────────────────────────────────────────────
if [ "$MODE" = "system" ]; then DEST="/usr/bin/hellish"
else DEST="$UPREFIX/bin/hellish"; fi
say "done. installed -> $DEST"
"$DEST" --version 2>/dev/null | head -1 || true
if [ "$MODE" = "user" ]; then
	say "open a new terminal to land in hellish (or run: $DEST)"
	say "undo any time:  sh install.sh --uninstall"
else
	say "log out and back in for the login shell (or run: exec $DEST --login)"
	say "undo any time:  make my-shell-uninstall   (from a checkout)"
fi

#!/bin/sh
# ============================================================================
# polish installer -- one command, both privilege worlds.
#
#   curl -fsSL https://raw.githubusercontent.com/Univers42/polish/main/install.sh | sh
#
# What it does, in order:
#   1. downloads the latest static release binary and verifies its sha256;
#   2. detects whether you can use sudo, and routes to the right install:
#        with sudo    -> /usr/bin/polish + /etc/shells + chsh
#                        (exactly `make my_shell`, via tools/register_shell.sh)
#        without sudo -> ~/.local/bin/polish + an exec hook in your rc
#                        (exactly `make user-install`, via user-install.sh)
#      Both drivers ship in a small "install bundle" published beside the
#      binary, so this script REUSES the proven scripts instead of
#      reimplementing them -- preflight, smoke-test, never-clobber seeding,
#      uninstall markers and all.
#   3. offers the plugin framework (github.com/Univers42/hellishrc_plugins)
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
#   --zshrc                  ALSO load your ~/.zshrc inside polish, in the
#                            zsh dialect (opt-in, best effort; off by default
#                            -- a .zshrc is code written for zsh)
#   --no-zshrc               switch an earlier install's import off without
#                            asking
#   --uninstall              undo a user-mode install (system: make my-shell-uninstall)
#
# Environment (mainly for tests -- docker/Dockerfile.installer drives these).
# Each is read as POLISH_<X>, else the pre-3.4 HELLISH_<X>:
#   POLISH_INSTALL_MODE      user|system         same as --user/--system
#   POLISH_INSTALL_PLUGINS   all|none|LIST       same as --plugins
#   POLISH_IMPORT_ZSHRC      yes|no              same as --zshrc/--no-zshrc
#   POLISH_RELEASE_BASE      base URL for release downloads
#                            (default https://github.com/Univers42/polish/releases)
#   POLISH_RAW_BASE          base URL for raw-file fallback
#   POLISH_PLUGINS_SRC       git URL, tarball URL or local dir for the framework
#
# From a source checkout this script uses the local files and, if present, the
# locally built binary -- so `sh install.sh` in the repo works offline.
#
# THE RENAME (hellish -> polish, 3.4.0). A machine that already has hellish
# keeps it: the drivers install over the existing /usr/bin/hellish or
# ~/.local/bin/hellish (the path passwd, /etc/shells and its updater name)
# and add a `polish` symlink beside it, and ~/.hellishrc stays the rc.
# Only a machine with no hellish gets /usr/bin/polish and ~/.polishrc.
# ============================================================================
set -eu

REPO="Univers42/polish"
# Every release from 3.4.0 publishes both names with the same bytes; tags
# before it only the legacy one, which is what --version v3.3.0 needs.
ASSET="polish-linux-x86_64"
LEGACY_ASSET="hellish-linux-x86_64"
# Internal to the release, and the name older copies of this script ask
# for -- so it keeps the old spelling rather than being published twice.
BUNDLE="hellish-install-bundle.tar.gz"
# knob NAME DEFAULT: POLISH_NAME when it is set -- even empty, as in
# brand_getenv (src/brand/brand.c) -- else HELLISH_NAME; empty means DEFAULT.
# `:-` let an empty POLISH_X fall through to a stale HELLISH_X.
knob() {
	eval "_knob=\${POLISH_$1-\${HELLISH_$1-}}"
	printf '%s' "${_knob:-$2}"
}
RELEASE_BASE="$(knob RELEASE_BASE "https://github.com/$REPO/releases")"
RAW_BASE="$(knob RAW_BASE "https://raw.githubusercontent.com/$REPO/main")"
PLUGINS_SRC="$(knob PLUGINS_SRC https://github.com/Univers42/hellishrc_plugins)"

MODE="$(knob INSTALL_MODE "")"
PLUGINS="$(knob INSTALL_PLUGINS "")"
ZSHRC="$(knob IMPORT_ZSHRC "")"
ASSUME_YES=0
VERSION=""
LOGIN_SHELL=1
UPREFIX="${PREFIX:-$HOME/.local}"
ACTION="install"

say()  { printf '\033[1;36mpolish:\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mpolish:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mpolish:\033[0m %s\n' "$*" >&2; exit 1; }

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
	-h|--help) sed -n '2,56p' "$0" 2>/dev/null | sed 's/^# \{0,1\}//'; exit 0 ;;
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
# Only a REAL script path can mean "a source checkout". Under `curl | sh`
# there is no script file: $0 is "sh", dirname("sh") is ".", and the old
# `HERE=$(dirname "$0")` made whatever directory you happened to be in the
# checkout. `update --now` runs this one-liner in the shell's cwd -- so an
# update typed inside any hellish checkout installed THAT checkout's
# build/bin/hellish, however old, and never downloaded a thing. A 2.3.2
# came back that way onto a machine that had 2.8.7 in the release channel.
HERE=""
if [ -f "$0" ] && grep -Eq '^# (polish|hellish) installer' "$0" 2>/dev/null; then
	HERE="$(cd "$(dirname "$0")" && pwd)"
fi
WORK=""
cleanup() { [ -n "$WORK" ] && rm -rf "$WORK"; }
trap cleanup EXIT INT TERM

# The version a checkout SAYS it is, and the version its build actually IS.
# They drift the moment someone pulls without rebuilding.
# Both spellings throughout: the macro and the --version line say polish
# from 3.4.0, hellish before it, and either binary may be the one in hand.
checkout_version() {
	sed -n -e 's/^# *define POLISH_VERSION "\([^"]*\)".*/\1/p' \
		-e 's/^# *define HELLISH_VERSION "\([^"]*\)".*/\1/p' \
		"$1/incs/version.h" 2>/dev/null | head -1
}
quiet() {
	POLISH_NO_BANNER=1 HELLISH_NO_BANNER=1 \
	POLISH_NO_UPDATE_CHECK=1 HELLISH_NO_UPDATE_CHECK=1 \
	POLISH_NO_ANIM=1 HELLISH_NO_ANIM=1 "$@"
}
binary_version() {
	quiet "$1" --version </dev/null 2>/dev/null \
		| sed -n -e 's/^polish, version \([^ ]*\).*/\1/p' \
			-e 's/^hellish, version \([^ ]*\).*/\1/p'
}
# binary_brand BIN -> "hellish" for a pre-3.4 binary, which only knows the
# hellish config paths, else "polish".
binary_brand() {
	case "$(quiet "$1" --version </dev/null 2>/dev/null | head -1)" in
	hellish[,\ ]*) echo hellish ;;
	*) echo polish ;;
	esac
}
# same_file A B -> 0 when both resolve to one file (test -ef is not POSIX).
# An empty resolution (a readlink with no -f) must not make every pair equal.
same_file() {
	[ -e "$1" ] && [ -e "$2" ] || return 1
	_sf="$(readlink -f "$1" 2>/dev/null)"
	[ -n "$_sf" ] && [ "$_sf" = "$(readlink -f "$2" 2>/dev/null)" ]
}

SRC_ROOT=""
BIN=""
if [ -n "$HERE" ] && [ -f "$HERE/Makefile" ] && [ -x "$HERE/tools/register_shell.sh" ]; then
	# A source checkout: the drivers are right here. Its built binary wins
	# over a download ONLY if it is the version the checkout claims -- a
	# stale build/bin/polish left over from an old branch is exactly what
	# must never be installed by a script whose job is "give me the latest".
	SRC_ROOT="$HERE"
	_want="$(checkout_version "$HERE")"
	# build/bin/hellish is the pre-3.4 name (and, from 3.4, a symlink to
	# build/bin/polish, which the version check makes harmless).
	for _cand in "$HERE/build/bin/polish" "$HERE/build/bin/hellish" \
		"$HERE/dist/$ASSET" "$HERE/dist/$LEGACY_ASSET"; do
		[ -x "$_cand" ] || continue
		_have="$(binary_version "$_cand")"
		if [ -z "$_want" ] || [ "$_have" = "$_want" ]; then
			BIN="$_cand"
			break
		fi
		warn "$_cand is ${_have:-an unversioned build} but this checkout is $_want -- not installing it"
		warn "  (run 'make OPT=1' to rebuild it; fetching the release instead)"
	done
fi

WORK="$(mktemp -d)"

if [ -z "$BIN" ]; then
	OS="$(uname -s)"; ARCH="$(uname -m)"
	if [ "$OS" != "Linux" ] || [ "$ARCH" != "x86_64" ]; then
		warn "no prebuilt binary for $OS/$ARCH yet -- build from source:"
		warn "  git -c http.version=HTTP/1.1 clone --recursive https://github.com/$REPO"
		warn "  cd polish && make OPT=1 all"
		exit 1
	fi
	if [ -n "$VERSION" ]; then DL="$RELEASE_BASE/download/$VERSION"
	else DL="$RELEASE_BASE/latest/download"; fi
	say "downloading ${VERSION:-the latest release}..."
	# The checksum fetched is the one published for the asset that came
	# down, never the other name's.
	_got=""
	for _a in "$ASSET" "$LEGACY_ASSET"; do
		if fetch "$DL/$_a" "$WORK/shell.bin" 2>/dev/null; then
			_got="$_a"
			break
		fi
	done
	[ -n "$_got" ] || die "download failed: $DL/$ASSET (and $LEGACY_ASSET)"
	if fetch "$DL/$_got.sha256" "$WORK/shell.bin.sha256" 2>/dev/null \
		&& command -v sha256sum >/dev/null 2>&1; then
		( cd "$WORK" && sed "s/ .*/  shell.bin/" shell.bin.sha256 \
			| sha256sum -c - >/dev/null 2>&1 ) \
			|| die "sha256 verification FAILED -- refusing to install"
		say "sha256 verified ($_got)"
	else
		warn "could not verify the checksum (missing .sha256 or sha256sum)"
	fi
	chmod +x "$WORK/shell.bin"
	BIN="$WORK/shell.bin"
fi
if [ -n "$SRC_ROOT" ]; then
	case "$BIN" in
	"$SRC_ROOT"/build/bin/*)
		say "using the checkout's own build ($(binary_version "$BIN"))" ;;
	esac
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
	# The drivers are refreshed from main ON TOP of the bundle, every time.
	# This script is fetched from main, so main's drivers are the ones it
	# was written against; the bundle's are frozen at the release. A seeder
	# fix landed on main and every `curl | sh` still ran the broken copy
	# from the latest release's bundle -- the fix was unreachable without
	# cutting a release, and the release took longer than the next report.
	# Best effort: no network to raw (a mirror, an offline box) keeps the
	# bundle's copies, and the themes always come from the bundle.
	_fresh=0
	for f in user-install.sh tools/register_shell.sh tools/seed_hellishrc.sh \
		hellishrc.example; do
		if fetch "$RAW_BASE/$f" "$WORK/$f.main" 2>/dev/null \
			&& [ -s "$WORK/$f.main" ] && head -1 "$WORK/$f.main" | grep -q '^#'; then
			mv -f "$WORK/$f.main" "$WORK/$f"
			_fresh=$((_fresh + 1))
		else
			rm -f "$WORK/$f.main"
		fi
	done
	chmod +x "$WORK/user-install.sh" "$WORK"/tools/*.sh 2>/dev/null || true
	[ "$_fresh" = 4 ] || warn "could not refresh all install scripts from main -- using the release's copies"
	SRC_ROOT="$WORK"
fi

# ── uninstall (user mode; system mode has richer tooling in the repo) ───────
if [ "$ACTION" = "uninstall" ]; then
	sh "$SRC_ROOT/user-install.sh" --uninstall --prefix "$UPREFIX"
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

# ── where it lands ──────────────────────────────────────────────────────────
# The drivers decide (polish, or hellish + a polish symlink on a machine
# that already has hellish -- see their headers), and this script asks
# rather than guessing a second time. A driver too old to answer
# --print-dest (a pre-3.4 bundle, with main unreachable) is one that only
# ever installs the hellish name.
if [ "$MODE" = "system" ]; then
	DEST="$( cd "$SRC_ROOT" && sh tools/register_shell.sh --print-dest \
		--bin "$BIN" 2>/dev/null )" || DEST=""
	[ -n "$DEST" ] || DEST="/usr/bin/hellish"
else
	DEST="$( cd "$SRC_ROOT" && sh user-install.sh --print-dest \
		--bin "$BIN" --prefix "$UPREFIX" 2>/dev/null )" || DEST=""
	[ -n "$DEST" ] || DEST="$UPREFIX/bin/hellish"
fi
# The seeder learned --bin with the rename; an older one rejects it.
SEED_ARGS=""
grep -q -- '--bin' "$SRC_ROOT/tools/seed_hellishrc.sh" 2>/dev/null \
	&& SEED_ARGS="--bin"

# Taken before the seeder runs: a ~/.polishrc that exists NOW is the user's,
# and is never moved (only one this run seeds may be, for an old framework).
HAD_POLISHRC=0
[ -e "$HOME/.polishrc" ] && HAD_POLISHRC=1

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
		&& ! ask "Register polish as your login shell (chsh)?" y; then
		LOGIN_SHELL=0
	fi
	if [ -n "$SEED_ARGS" ]; then
		( cd "$SRC_ROOT" && sh tools/seed_hellishrc.sh --bin "$BIN" )
	else
		( cd "$SRC_ROOT" && sh tools/seed_hellishrc.sh )
	fi
	if [ "$LOGIN_SHELL" = "1" ]; then
		( cd "$SRC_ROOT" && sh tools/register_shell.sh \
			--bin "$BIN" --dest "$DEST" )
	else
		_as_root=""; [ "$(id -u)" = "0" ] || _as_root="sudo"
		$_as_root install -m 0755 "$BIN" "$DEST"
		# Same pairing as register_shell.sh: one binary, and the new name
		# beside a kept hellish as a symlink -- never a second copy.
		_al="$(dirname "$DEST")/polish"
		if [ "$(basename "$DEST")" = "hellish" ] && [ ! -d "$_al" ]; then
			$_as_root ln -sf hellish "$_al.new" \
				&& $_as_root mv -f "$_al.new" "$_al"
		fi
		say "installed $DEST (login shell untouched; run: polish)"
	fi
else
	( cd "$SRC_ROOT" && sh user-install.sh --bin "$BIN" --prefix "$UPREFIX" )
fi

# ── your zsh configuration stays zsh's ──────────────────────────────────────
# (The history below is from before the rename; "hellish" is the shell.)
# ~/.zshrc is code written for zsh, and hellish does not run it. It did, for
# three releases (2.8.7 to 2.9.0): the installer offered to load the file
# inside hellish in the zsh dialect, defaulted to yes, and every 42 account
# said yes. One .zshrc began with `exec /bin/bash`, so hellish replaced
# itself with bash on every start (#116); one ran promptinit, compinit and
# twenty zstyles (#115); one loaded oh-my-zsh, whose guard refuses any shell
# that is not zsh (#114); one had a line hellish could not parse (#113).
# Each was a config that worked in zsh, run by a shell that is not zsh.
# Aliases and git shortcuts come to hellish through the plugin framework
# and ~/.hellishrc; the zsh config is left to zsh.
#
# --zshrc is the explicit, best-effort opt-in: the file is loaded in the
# zsh dialect from after.d, AFTER ~/.hellishrc, so it keeps the last word
# over the framework (an alias there beats a function here, not the other
# way round). An import left by the earlier default is switched off on
# re-run -- renamed .off, never deleted -- unless --zshrc says to keep it,
# and either way the user is told.
login_shell_name() {
	_ls=""
	if command -v getent >/dev/null 2>&1; then
		_ls="$(getent passwd "$(id -un)" 2>/dev/null | cut -d: -f7)"
	fi
	[ -n "$_ls" ] || _ls="${SHELL:-/bin/sh}"
	basename "$_ls"
}
case "${XDG_CONFIG_HOME:-}" in
"$HOME"/*) XDG_BASE="$XDG_CONFIG_HOME" ;;
*) XDG_BASE="$HOME/.config" ;;
esac
# The shell's own rule (src/brand): the polish dir unless only the hellish
# one exists. A pre-3.4 binary reads only the hellish one.
if [ "$(binary_brand "$BIN")" = "hellish" ] \
	|| { [ -d "$XDG_BASE/hellish" ] && [ ! -e "$XDG_BASE/polish" ]; }; then
	XDG="$XDG_BASE/hellish"
else
	XDG="$XDG_BASE/polish"
fi
ZSHRC_MODULE="$XDG/after.d/90-zshrc.zsh"
LEGACY_MODULE="$XDG/rc.d/90-zshrc.zsh"
if [ "$ZSHRC" = "yes" ]; then
	mkdir -p "$XDG/after.d"
	if [ -e "$LEGACY_MODULE" ] && [ ! -e "$ZSHRC_MODULE" ]; then
		mv "$LEGACY_MODULE" "$ZSHRC_MODULE"
		say "moved the ~/.zshrc import to after.d, so it loads after your rc"
	fi
	[ -e "$LEGACY_MODULE" ] && mv "$LEGACY_MODULE" "$LEGACY_MODULE.off"
	if [ -e "$ZSHRC_MODULE" ]; then
		say "keeping $ZSHRC_MODULE"
	else
		cat > "$ZSHRC_MODULE" <<'EOF'
# after.d/90-zshrc.zsh -- written by `install.sh --zshrc`.
#
# Loads your ~/.zshrc inside polish, BEST EFFORT. The .zsh extension is
# what makes it work at all: polish reads this file -- and everything it
# sources -- in the zsh dialect, so `precmd() { vcs_info }`, zstyle,
# PROMPT/RPROMPT and the rest of a zsh config run as written. Anything
# zsh-only that polish cannot do (compinit, zle widgets) is reported once
# and skipped; `source $ZSH/oh-my-zsh.sh` loads the plugins your
# `plugins=(...)` names and keeps polish's prompt. A .zshrc is still code
# written for another shell: if it misbehaves here, delete this file (or
# re-run the installer, which switches it off) and copy the lines you want
# into your rc (~/.polishrc, or ~/.hellishrc on an older install) instead.
#
# after.d is sourced AFTER the rc and the plugin framework, so what
# you set here has the last word, exactly as ~/.zshrc does in zsh.
if [ -f "$HOME/.zshrc" ]; then
	# The block the installer appended to ~/.zshrc execs the shell from an
	# interactive zsh. These markers tell it not to do so again from inside
	# the shell itself -- both, because the block may predate the rename.
	POLISH_EXECD=1
	HELLISH_EXECD=1
	export POLISH_EXECD HELLISH_EXECD
	source "$HOME/.zshrc"
fi
EOF
		say "your ~/.zshrc will load inside polish (zsh dialect, best effort): $ZSHRC_MODULE"
	fi
else
	for _m in "$LEGACY_MODULE" "$ZSHRC_MODULE"; do
		[ -e "$_m" ] || continue
		if [ "$ZSHRC" != "no" ] && [ "$INTERACTIVE" = "1" ] \
			&& ask "An earlier install loads your ~/.zshrc inside the shell (best effort). Keep that?" n; then
			if [ "$_m" = "$LEGACY_MODULE" ]; then
				mkdir -p "$XDG/after.d"
				mv "$_m" "$ZSHRC_MODULE"
				say "moved the ~/.zshrc import to after.d, so it loads after your rc"
			else
				say "keeping $_m"
			fi
		else
			mv "$_m" "$_m.off"
			say "your ~/.zshrc is no longer loaded inside polish: $_m -> .off  (opt back in: install.sh --zshrc)"
		fi
	done
	if [ -f "$HOME/.zshrc" ] && [ "$(login_shell_name)" = "zsh" ]; then
		say "your ~/.zshrc stays zsh's; aliases you want in polish go in your rc"
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
	&& ask "Install the plugin framework (git, jump, docker, net + a plugin catalog)?" y; then
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
	# A polish-aware framework (it names ~/.polishrc) installs into the rc
	# the shell reads and picks shebangs by `command -v polish`, so it runs
	# with the new binary's dir on PATH -- that is how a fresh install ends
	# up with no hellish name at all. An older framework only knows
	# ~/.hellishrc, which the shell ignores while ~/.polishrc exists: a
	# polishrc this run seeded moves there (the framework keeps its content
	# as rc.d/95-previous-rc.hsh); one the user already had is not ours to
	# move, so that framework waits.
	# Caveat: "polish-aware" is a grep for .polishrc in its installer -- a
	# fork that only mentions the name in a comment is misread as aware.
	if ! grep -q '\.polishrc' "$FW/install.sh" 2>/dev/null; then
		if [ "$HAD_POLISHRC" = "1" ]; then
			want_framework=0
			say "plugin framework skipped: this version installs into ~/.hellishrc, which polish ignores while your ~/.polishrc exists"
		elif [ -f "$HOME/.polishrc" ] && [ ! -e "$HOME/.hellishrc" ]; then
			mv "$HOME/.polishrc" "$HOME/.hellishrc"
			say "moved ~/.polishrc to ~/.hellishrc, where this framework version installs (still read by polish)"
		fi
	fi
	if [ "$want_framework" = "1" ]; then
		PATH="$(dirname "$DEST"):$PATH" sh "$FW/install.sh" \
			--plugins "${PLUGINS:-all}"
	fi
fi

# ── report ──────────────────────────────────────────────────────────────────
say "done. installed -> $DEST"
quiet "$DEST" --version </dev/null 2>/dev/null | head -1 || true

# Another copy on PATH is how a fresh install still greets you with an
# old banner: a 2.3.2 forgotten in ~/bin, ahead of ~/.local/bin, answered
# `hellish` on a machine that had just installed 2.8.7. Name every other
# copy, under either name, and shout about the ones that come FIRST. The
# polish symlink beside a kept hellish resolves to DEST and is not "other".
_seen=0
IFS=:; for _d in $PATH; do
	unset IFS
	[ -n "$_d" ] || continue
	for _b in "$_d/polish" "$_d/hellish"; do
		[ -x "$_b" ] || continue
		if same_file "$_b" "$DEST"; then _seen=1; continue; fi
		_v="$(binary_version "$_b")"
		if [ "$_seen" = "0" ]; then
			warn "another copy is AHEAD of $DEST on your PATH: $_b (${_v:-unknown version})"
			warn "  \`$(basename "$_b")\` will keep running that one -- remove it, or move $(dirname "$DEST") before $_d in PATH"
		else
			say "note: another copy on PATH: $_b (${_v:-unknown version})"
		fi
	done
done; unset IFS
if [ "$MODE" = "user" ]; then
	say "open a new terminal to land in polish (or run: $DEST)"
	say "undo any time:  sh install.sh --uninstall"
else
	say "log out and back in for the login shell (or run: exec $DEST --login)"
	say "undo any time:  make my-shell-uninstall   (from a checkout)"
fi

#!/bin/sh
# hellish user-install — make hellish your default shell WITHOUT root.
#
#   ./user-install.sh              install (or re-install; idempotent)
#   ./user-install.sh --uninstall  put things back exactly as they were
#
# Normally `make user-install` / `make user-uninstall` drives this.
#
# WHY THIS EXISTS
# ---------------
# The usual route is chsh(1), and chsh refuses point-blank to set a shell that
# is not listed in /etc/shells. Only root writes that file. On a lab machine,
# a shared box, or any account you do not own, that route is simply closed —
# which is what `make my_shell` needs sudo for.
#
# The sudo-less route is the one every user already has: the login shell you
# are stuck with reads an rc file when it starts, and that rc file can hand the
# terminal over with `exec`. exec REPLACES the process, so this is not a
# wrapper: `ps` shows hellish, $$ is hellish's pid, closing hellish closes the
# tab, and there is no stray bash sitting underneath eating a signal. It is
# also not an alias — you open a terminal and you are IN hellish, without
# typing anything.
#
# The passwd entry stays untouched, and that is a feature, not a shortcut: it
# is what keeps `ssh host 'some command'` working (non-interactive, so the hook
# never fires) and leaves you a guaranteed way back in if hellish ever wedges.
#
# WHAT IT TOUCHES
# ---------------
#   1. $PREFIX/bin/hellish        the binary                 (default ~/.local)
#   2. ~/.hellishrc               seeded from hellishrc.example, NEVER clobbered,
#                                 plus one marker-delimited block that puts
#                                 $PREFIX/bin on PATH
#   3. <your login shell's rc>    one marker-delimited block, appended at the end
#
# Nothing else. Re-running replaces the block in place rather than stacking a
# second copy, so this is safe to run on every rebuild.

set -eu

PROG="$(basename "$0")"
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

PREFIX="${PREFIX:-$HOME/.local}"
BIN_SRC="${BIN_SRC:-$REPO_ROOT/build/bin/hellish}"
RC_TARGET="${RC_TARGET:-}"
ACTION="install"

BEGIN_MARK="# >>> hellish >>>"
END_MARK="# <<< hellish <<<"

# ── plumbing ──────────────────────────────────────────────────────────────
red()  { printf '\033[1;31m%s\033[0m\n' "$*" >&2; }
grn()  { printf '  \033[1;32m✓\033[0m %s\n' "$*" >&2; }
inf()  { printf '  \033[1;36m▸\033[0m %s\n' "$*" >&2; }
warn() { printf '  \033[1;33m!\033[0m %s\n' "$*" >&2; }
die()  { red "$PROG: $*"; exit 1; }

usage() {
	cat >&2 <<EOF
usage: $PROG [--uninstall] [--prefix DIR] [--bin PATH] [--rc FILE]

  --prefix DIR   where to put bin/hellish      (default: \$HOME/.local)
  --bin PATH     which binary to install       (default: build/bin/hellish)
  --rc FILE      rc file to hook               (default: your login shell's)
  --uninstall    remove the hook and the installed binary
EOF
	exit 2
}

while [ $# -gt 0 ]; do
	case "$1" in
		--uninstall) ACTION="uninstall" ;;
		--prefix) shift; [ $# -gt 0 ] || usage; PREFIX="$1" ;;
		--bin)    shift; [ $# -gt 0 ] || usage; BIN_SRC="$1" ;;
		--rc)     shift; [ $# -gt 0 ] || usage; RC_TARGET="$1" ;;
		-h|--help) usage ;;
		*) red "$PROG: unknown argument '$1'"; usage ;;
	esac
	shift
done

# `make user-install PREFIX=~/opt` hands us a literal tilde: make does not
# expand it, and neither does the quoted assignment that follows. Do it here so
# nobody ends up with a directory actually named '~'.
untilde() {
	case "$1" in
		"~")   printf '%s\n' "$HOME" ;;
		"~/"*) printf '%s\n' "$HOME/${1#\~/}" ;;
		*)     printf '%s\n' "$1" ;;
	esac
}
PREFIX="$(untilde "$PREFIX")"
BIN_SRC="$(untilde "$BIN_SRC")"
if [ -n "$RC_TARGET" ]; then
	RC_TARGET="$(untilde "$RC_TARGET")"
fi

DEST_DIR="$PREFIX/bin"
DEST="$DEST_DIR/hellish"

# ── which rc file does THIS user's login shell actually read? ─────────────
#
# We hook the *interactive* rc, not the profile. Two reasons. It is the file
# that only ever runs for a human at a terminal, and — more importantly —
# ~/.profile is also read by some display managers when the graphical session
# starts. An exec there would replace your desktop session's shell. The
# `case $- in *i*` guard in the hook already covers that, but not editing the
# file at all is the belt to that suspenders.
login_shell() {
	_ls=""
	if command -v getent >/dev/null 2>&1; then
		_ls="$(getent passwd "$(id -un)" 2>/dev/null | cut -d: -f7)"
	fi
	[ -n "$_ls" ] || _ls="${SHELL:-/bin/sh}"
	basename "$_ls"
}

rc_for_shell() {
	case "$1" in
		bash)            printf '%s\n' "$HOME/.bashrc" ;;
		zsh)             printf '%s\n' "${ZDOTDIR:-$HOME}/.zshrc" ;;
		ksh|ksh93|mksh)  printf '%s\n' "${ENV:-$HOME/.kshrc}" ;;
		sh|dash|ash)     printf '%s\n' "$HOME/.profile" ;;
		fish)            printf '%s\n' "" ;;
		*)               printf '%s\n' "" ;;
	esac
}

LOGIN_SHELL="$(login_shell)"
if [ -z "$RC_TARGET" ]; then
	RC_TARGET="$(rc_for_shell "$LOGIN_SHELL")"
fi
if [ -z "$RC_TARGET" ]; then
	red "$PROG: don't know which rc file '$LOGIN_SHELL' reads."
	if [ "$LOGIN_SHELL" = "fish" ]; then
		red "fish uses its own syntax; add this to ~/.config/fish/config.fish:"
		red "    if status is-interactive; and not set -q HELLISH_EXECD"
		red "        set -x HELLISH_EXECD 1; set -x SHELL $DEST; exec $DEST"
		red "    end"
	fi
	die "point me at one explicitly:  $PROG --rc ~/.somerc"
fi

# Delete an existing hook block from stdin. Written so a half-removed block
# (someone deleted one marker by hand) still degrades to "leave it alone"
# rather than eating the rest of the file.
strip_block() {
	awk -v b="$BEGIN_MARK" -v e="$END_MARK" '
		$0 == e { skip = 0; next }
		$0 == b { skip = 1 }
		skip    { next }
		        { print }
	'
}

has_block() {
	[ -f "$1" ] && grep -qxF "$BEGIN_MARK" "$1"
}

# Two older generations of this block exist in the wild -- `hellish-default`
# in ~/.bashrc and `hellish-user-default` in ~/.zshrc, from the 2.7.x
# installers, with a HELLISH_STARTED guard of their own. A machine that has
# seen three installs carries all three (issue #116), and `make
# user-uninstall` left the old ones behind, so hellish kept coming up after
# it was "removed". They are ours, so they go: exact marker pairs only.
LEGACY_RE='^# (>>>|<<<) hellish-(user-)?default (>>>|<<<)$'
strip_legacy_blocks() {
	awk 'BEGIN { skip = 0 }
	     /^# <<< hellish-(user-)?default <<<$/ { skip = 0; next }
	     /^# >>> hellish-(user-)?default >>>$/ { skip = 1 }
	     skip    { next }
	     { print }'
}

# Remove every legacy block from FILE, with a backup beside it.
drop_legacy_hooks() {
	[ -f "$1" ] && grep -qE "$LEGACY_RE" "$1" || return 0
	cp -p "$1" "$1.hellish-legacy-bak"
	strip_legacy_blocks < "$1.hellish-legacy-bak" | trim_trailing_blanks > "$1.tmp"
	mv "$1.tmp" "$1"
	grn "removed an older hellish hook from $1 (backup: $1.hellish-legacy-bak)"
}

# Drop trailing blank lines. We insert a blank line ahead of the block, so
# without this an install/uninstall/install cycle would slowly grow a gap at
# the end of the rc file. Interior blank lines are preserved.
trim_trailing_blanks() {
	awk 'BEGIN { n = 0 }
	     /^[[:space:]]*$/ { n++; next }
	     { while (n-- > 0) print ""; n = 0; print }'
}

# ── uninstall ─────────────────────────────────────────────────────────────
if [ "$ACTION" = "uninstall" ]; then
	if has_block "$RC_TARGET"; then
		cp -p "$RC_TARGET" "$RC_TARGET.hellish-bak"
		strip_block < "$RC_TARGET.hellish-bak" \
			| trim_trailing_blanks > "$RC_TARGET.hellish-new"
		mv "$RC_TARGET.hellish-new" "$RC_TARGET"
		grn "removed the hook from $RC_TARGET (backup: $RC_TARGET.hellish-bak)"
	else
		inf "no hook found in $RC_TARGET — nothing to undo there"
	fi
	for _rc in "$RC_TARGET" "$HOME/.bashrc" "$HOME/.zshrc"; do
		drop_legacy_hooks "$_rc"
	done
	if [ -e "$DEST" ]; then
		rm -f "$DEST"
		grn "removed $DEST"
	fi
	# The rc file is the user's; only the block THIS script wrote comes out.
	"$REPO_ROOT/tools/seed_hellishrc.sh" --strip-path >&2 || true
	inf "~/.hellishrc left in place (it is yours; delete it by hand if you want)"
	printf '\n  Open a NEW terminal and you are back in %s.\n\n' "$LOGIN_SHELL" >&2
	exit 0
fi

# ── 1. install the binary ─────────────────────────────────────────────────
[ -x "$BIN_SRC" ] || die "no binary at $BIN_SRC — run 'make OPT=1' first"

mkdir -p "$DEST_DIR"
# Install to a temp name and rename. A plain `cp` over $DEST fails with
# ETXTBSY when the file being overwritten is the shell you are running this
# from — which is exactly what happens the second time you run this. rename(2)
# swaps the directory entry and leaves the running process on the old inode.
cp "$BIN_SRC" "$DEST.new"
chmod 755 "$DEST.new"
mv -f "$DEST.new" "$DEST"
grn "installed $DEST"

# ── 2. seed ~/.hellishrc ──────────────────────────────────────────────────
# Delegated, not inlined. This is the copy `make my_shell` did not have, so
# the sudo route installed a shell with no config at all (issue #51); one
# seeder called by both routes is what keeps them from drifting again.
# --path-dir is the other half of "install it and it works": $DEST_DIR is
# NOT necessarily on PATH (see the block the seeder writes), so without it
# the shell starts but its own name does not resolve.
"$REPO_ROOT/tools/seed_hellishrc.sh" --example "$REPO_ROOT/hellishrc.example" \
	--path-dir "$DEST_DIR"

# ── 3. prove the binary works BEFORE anything execs it ────────────────────
#
# This is the whole safety story. Once the hook is in place, every new
# terminal runs this binary as its shell; if it is broken, terminals die on
# open and you have a genuinely annoying afternoon. So the hook is only ever
# written after the installed binary has demonstrably run a command, honoured
# an exit status, and accepted input on a pipe.
inf "smoke-testing $DEST"
smoke_env() {
	HELLISH_NO_BANNER=1 HELLISH_NO_ANIM=1 HELLISH_NO_UPDATE_CHECK=1 "$@"
}
smoke_st=0
smoke_env "$DEST" -c 'exit 42' >/dev/null 2>&1 || smoke_st=$?
[ "$smoke_st" = "42" ] || die "smoke test failed: '$DEST -c' returned $smoke_st, want 42"
smoke_st=0
printf 'exit 7\n' | smoke_env "$DEST" >/dev/null 2>&1 || smoke_st=$?
[ "$smoke_st" = "7" ] || die "smoke test failed: piped input returned $smoke_st, want 7"
grn "binary runs commands and reports status correctly"

# A broken ~/.hellishrc cannot stop the shell from starting (it is sourced,
# not exec'd), so this is a warning and not a hard failure — but you want to
# hear about it now rather than wonder why your aliases are missing.
if [ -f "$HOME/.hellishrc" ]; then
	if ! smoke_env "$DEST" -c '. "$HOME/.hellishrc"' >/dev/null 2>&1; then
		warn "~/.hellishrc reported errors when sourced — check it"
	fi
fi

# ── 4. write the hook ─────────────────────────────────────────────────────
# The rc being hooked loses any 2.7.x block first: the one written below
# supersedes it. Other rc files keep theirs until uninstall -- a hook in
# ~/.bashrc may be the only reason a zsh user who execs bash ever reaches
# hellish (issue #116), and that is their arrangement to change.
[ -e "$RC_TARGET" ] || : > "$RC_TARGET"
drop_legacy_hooks "$RC_TARGET"
cp -p "$RC_TARGET" "$RC_TARGET.hellish-bak"
strip_block < "$RC_TARGET.hellish-bak" | trim_trailing_blanks > "$RC_TARGET.tmp"

cat >> "$RC_TARGET.tmp" <<EOF

$BEGIN_MARK
# Added by hellish's user-install.sh. Everything between the markers is
# regenerated on re-install and deleted by \`make user-uninstall\` — put your
# own edits outside them.
#
# chsh(1) will not point at a shell that is missing from /etc/shells, and
# only root writes that file. So the handover happens here instead: an
# interactive shell replaces ITSELF with hellish. exec means no wrapper
# process is left behind — hellish owns the terminal, the pid and the exit.
#
# This block sits at the END of the file on purpose: whatever you set up
# above (PATH, nvm, proxies) is already in the environment, and exec carries
# the environment across, so hellish starts with the setup you just built.
#
# Ways out, in increasing order of permanence:
#   HELLISH_NO_EXEC=1 <terminal>    one session in $LOGIN_SHELL
#   touch ~/.hellish-disable        every new session, until you delete it
#   make user-uninstall             remove this block for good
# And from another machine, since your passwd shell is untouched:
#   ssh $(id -un)@\$(hostname) 'touch ~/.hellish-disable'

# The binary lives in a directory your login chain may not have on PATH:
# ~/.profile adds ~/.local/bin only when it already exists at login, and on a
# first install this run is what created it. The exec below uses an absolute
# path, so the shell starts either way — but without this, \`hellish\` as a
# COMMAND ("hellish update", "command -v hellish", any tool that looks the
# shell up by name) is not found, here or inside hellish. Guarded, because
# an rc gets re-sourced and an unguarded prepend stacks duplicates.
case ":\$PATH:" in
	*":$DEST_DIR:"*) ;;
	*) PATH="$DEST_DIR:\$PATH"; export PATH ;;
esac

if [ -z "\${HELLISH_NO_EXEC-}" ] && [ -z "\${HELLISH_EXECD-}" ] \\
	&& [ ! -e "\$HOME/.hellish-disable" ] && [ -x "$DEST" ]; then
	case \$- in
	*i*)
		# Exported BEFORE the exec, so hellish and everything under it
		# inherits the marker. That is what lets you type \`$LOGIN_SHELL\`
		# inside hellish and actually get $LOGIN_SHELL instead of bouncing
		# straight back here.
		HELLISH_EXECD=1
		export HELLISH_EXECD
		# So \$SHELL-respecting tools (tmux, vim's :shell, xdg-terminal)
		# spawn hellish too, which is the other half of "default shell".
		SHELL="$DEST"
		export SHELL
		exec "$DEST"
		;;
	esac
fi
$END_MARK
EOF

# Refuse to install a syntactically broken rc: a bad rc file greets you with
# errors on every single terminal, and this script is the one that touched it.
if command -v "$LOGIN_SHELL" >/dev/null 2>&1; then
	if ! "$LOGIN_SHELL" -n "$RC_TARGET.tmp" 2>/dev/null; then
		rm -f "$RC_TARGET.tmp"
		die "generated rc would not parse under $LOGIN_SHELL — aborted, $RC_TARGET untouched"
	fi
fi

mv "$RC_TARGET.tmp" "$RC_TARGET"
grn "hooked $RC_TARGET (backup: $RC_TARGET.hellish-bak)"

# ── done ──────────────────────────────────────────────────────────────────
printf '
  \033[1;37mhellish is now your default shell.\033[0m Open a new terminal — no logout needed.
  In THIS one, right now:   \033[1;36mexec %s\033[0m

  Your config lives in \033[1;36m~/.hellishrc\033[0m (sourced by every interactive
  hellish, never by scripts or -c). Shell functions do not survive exec, so
  anything you had as a %s function — nvm, rbenv, conda — belongs in there
  too.

  Undo everything:          \033[1;36mmake user-uninstall\033[0m

' "$DEST" "$LOGIN_SHELL" >&2

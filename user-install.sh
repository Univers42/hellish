#!/bin/sh
# polish user-install — make polish your default shell WITHOUT root.
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
# wrapper: `ps` shows polish, $$ is polish's pid, closing polish closes the
# tab, and there is no stray bash sitting underneath eating a signal. It is
# also not an alias — you open a terminal and you are IN hellish, without
# typing anything.
#
# The passwd entry stays untouched, and that is a feature, not a shortcut: it
# is what keeps `ssh host 'some command'` working (non-interactive, so the hook
# never fires) and leaves you a guaranteed way back in if polish ever wedges.
#
# WHAT IT TOUCHES
# ---------------
#   1. $PREFIX/bin/polish         the binary                 (default ~/.local)
#   2. ~/.polishrc                seeded from hellishrc.example, NEVER clobbered,
#                                 plus one marker-delimited block that puts
#                                 $PREFIX/bin on PATH
#   3. <your login shell's rc>    one marker-delimited block, appended at the end
#
# Nothing else. Re-running replaces the block in place rather than stacking a
# second copy, so this is safe to run on every rebuild.
#
# THE RENAME (hellish -> polish, 3.4.0)
# -------------------------------------
# A machine that already runs hellish keeps its paths: if $PREFIX/bin/hellish
# exists, or the rc already carries the `# >>> hellish >>>` block, or the
# binary being installed is itself a pre-3.4 hellish, the binary is
# (re)installed as $PREFIX/bin/hellish and $PREFIX/bin/polish becomes a
# symlink to it. The rc hook, the PATH and the binary's own updater all name
# the old path, and two real binaries would mean `update` refreshes one while
# the other goes stale -- so the new name is only ever an alias there.

set -eu

PROG="$(basename "$0")"
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

PREFIX="${PREFIX:-$HOME/.local}"
if [ -z "${BIN_SRC:-}" ]; then
	BIN_SRC="$REPO_ROOT/build/bin/polish"
	[ -x "$BIN_SRC" ] || BIN_SRC="$REPO_ROOT/build/bin/hellish"
fi
RC_TARGET="${RC_TARGET:-}"
ACTION="install"

LEGACY_BEGIN="# >>> hellish >>>"
LEGACY_END="# <<< hellish <<<"
BEGIN_MARK="# >>> polish >>>"
END_MARK="# <<< polish <<<"

# ── plumbing ──────────────────────────────────────────────────────────────
red()  { printf '\033[1;31m%s\033[0m\n' "$*" >&2; }
grn()  { printf '  \033[1;32m✓\033[0m %s\n' "$*" >&2; }
inf()  { printf '  \033[1;36m▸\033[0m %s\n' "$*" >&2; }
warn() { printf '  \033[1;33m!\033[0m %s\n' "$*" >&2; }
die()  { red "$PROG: $*"; exit 1; }

usage() {
	cat >&2 <<EOF
usage: $PROG [--uninstall|--print-dest] [--prefix DIR] [--bin PATH] [--rc FILE]

  --prefix DIR   where to put bin/polish       (default: \$HOME/.local)
  --bin PATH     which binary to install       (default: build/bin/polish)
  --rc FILE      rc file to hook               (default: your login shell's)
  --uninstall    remove the hook and the installed binary
  --print-dest   print the path the binary would be installed at, and exit
                 (bin/hellish on a machine that already has a hellish install)
EOF
	exit 2
}

while [ $# -gt 0 ]; do
	case "$1" in
		--uninstall) ACTION="uninstall" ;;
		--print-dest) ACTION="print-dest" ;;
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
		\~)    printf '%s\n' "$HOME" ;;
		\~/*)  printf '%s\n' "$HOME/${1#\~/}" ;;
		*)     printf '%s\n' "$1" ;;
	esac
}
PREFIX="$(untilde "$PREFIX")"
BIN_SRC="$(untilde "$BIN_SRC")"
if [ -n "$RC_TARGET" ]; then
	RC_TARGET="$(untilde "$RC_TARGET")"
fi

DEST_DIR="$PREFIX/bin"
LEGACY_DEST="$DEST_DIR/hellish"
ALIAS=""

# Every child we run gets both spellings of the quiet knobs: a 3.4.0+ binary
# reads POLISH_* first, an older one only knows HELLISH_*.
quiet() {
	POLISH_NO_BANNER=1 HELLISH_NO_BANNER=1 \
	POLISH_NO_UPDATE_CHECK=1 HELLISH_NO_UPDATE_CHECK=1 \
	POLISH_NO_ANIM=1 HELLISH_NO_ANIM=1 "$@"
}

# ── which rc file does THIS user's login shell actually read? ─────────────
#
# We hook the *interactive* rc, not the profile. Two reasons. It is the file
# that only ever runs for a human at a terminal, and — more importantly —
# ~/.profile is also read by some display managers when the graphical session
# starts. An exec there would replace your desktop session's shell. The
# `case $- in *i*` guard in the hook already covers that, but not editing the
# file at all is the belt to that suspenders.
login_shell_path() {
	_ls=""
	if command -v getent >/dev/null 2>&1; then
		_ls="$(getent passwd "$(id -un)" 2>/dev/null | cut -d: -f7)"
	fi
	[ -n "$_ls" ] || _ls="${SHELL:-/bin/sh}"
	printf '%s\n' "$_ls"
}
login_shell() { basename "$(login_shell_path)"; }

# same_file A B -> 0 when both exist and resolve to one file. readlink -f
# rather than test -ef, which POSIX sh does not define.
# An empty resolution (a readlink with no -f) must not make every pair equal.
same_file() {
	[ -e "$1" ] && [ -e "$2" ] || return 1
	_sf="$(readlink -f "$1" 2>/dev/null)"
	[ -n "$_sf" ] && [ "$_sf" = "$(readlink -f "$2" 2>/dev/null)" ]
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

# ── which name does this install go by? (see THE RENAME, above) ───────────
is_legacy_install() {
	[ -e "$LEGACY_DEST" ] || [ -L "$LEGACY_DEST" ] && return 0
	[ -n "$RC_TARGET" ] && [ -f "$RC_TARGET" ] \
		&& grep -qxF "$LEGACY_BEGIN" "$RC_TARGET" && return 0
	[ "$ACTION" != "uninstall" ] && [ -x "$BIN_SRC" ] || return 1
	case "$(quiet "$BIN_SRC" --version </dev/null 2>/dev/null | head -1)" in
	hellish[,\ ]*) return 0 ;;
	esac
	return 1
}
if is_legacy_install; then
	DEST="$LEGACY_DEST"
	ALIAS="$DEST_DIR/polish"
else
	DEST="$DEST_DIR/polish"
fi
if [ "$ACTION" = "print-dest" ]; then
	printf '%s\n' "$DEST"
	exit 0
fi

if [ -z "$RC_TARGET" ]; then
	red "$PROG: don't know which rc file '$LOGIN_SHELL' reads."
	if [ "$LOGIN_SHELL" = "fish" ]; then
		red "fish uses its own syntax; add this to ~/.config/fish/config.fish:"
		red "    if status is-interactive; and not set -q POLISH_EXECD; and not set -q HELLISH_EXECD"
		red "        set -x POLISH_EXECD 1; set -x HELLISH_EXECD 1"
		red "        set -x SHELL $DEST; exec $DEST"
		red "    end"
	fi
	die "point me at one explicitly:  $PROG --rc ~/.somerc"
fi

# Delete an existing hook block from stdin, under either marker spelling: an
# rc must never carry two exec hooks. Written so a half-removed block
# (someone deleted one marker by hand) still degrades to "leave it alone"
# rather than eating the rest of the file.
strip_block() {
	awk -v b="$BEGIN_MARK" -v e="$END_MARK" \
		-v lb="$LEGACY_BEGIN" -v le="$LEGACY_END" '
		$0 == e || $0 == le { skip = 0; next }
		$0 == b || $0 == lb { skip = 1 }
		skip    { next }
		        { print }
	'
}

has_block() {
	[ -f "$1" ] && grep -qxF -e "$BEGIN_MARK" -e "$LEGACY_BEGIN" "$1"
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
		cp -p "$RC_TARGET" "$RC_TARGET.polish-bak"
		strip_block < "$RC_TARGET.polish-bak" \
			| trim_trailing_blanks > "$RC_TARGET.polish-new"
		mv "$RC_TARGET.polish-new" "$RC_TARGET"
		grn "removed the hook from $RC_TARGET (backup: $RC_TARGET.polish-bak)"
	else
		inf "no hook found in $RC_TARGET — nothing to undo there"
	fi
	for _rc in "$RC_TARGET" "$HOME/.bashrc" "$HOME/.zshrc"; do
		drop_legacy_hooks "$_rc"
	done
	# Both names: either may be the binary, the other its alias. Neither
	# goes while it is the account's login shell (a chsh someone did by
	# hand) -- deleting that is a lockout only root can undo.
	_login="$(login_shell_path)"
	for _b in "$DEST_DIR/polish" "$DEST_DIR/hellish"; do
		[ -e "$_b" ] || [ -L "$_b" ] || continue
		if [ "$_login" = "$_b" ] || same_file "$_login" "$_b"; then
			warn "$_b is your login shell ($_login) — NOT removing it"
			warn "  chsh to another shell first, then re-run the uninstall"
			continue
		fi
		rm -f "$_b"
		grn "removed $_b"
	done
	# The rc file is the user's; only the block THIS script wrote comes out.
	"$REPO_ROOT/tools/seed_hellishrc.sh" --strip-path >&2 || true
	for _rc in "$HOME/.polishrc" "$HOME/.hellishrc"; do
		[ -f "$_rc" ] && inf "$_rc left in place (it is yours; delete it by hand if you want)"
	done
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

# The new name on an old install: a relative symlink, swapped in by rename
# for the same ETXTBSY reason as above. It replaces whatever is there --
# including a stray real polish binary, which is exactly the second copy that
# must not exist -- except a directory, which is not ours to touch.
if [ -n "$ALIAS" ]; then
	if [ -d "$ALIAS" ]; then
		warn "$ALIAS is a directory — not linking polish to $DEST"
	else
		rm -f "$ALIAS.new"
		ln -s hellish "$ALIAS.new"
		mv -f "$ALIAS.new" "$ALIAS"
		grn "linked $ALIAS -> hellish (the new name; one binary)"
	fi
fi

# ── 2. seed the rc (~/.polishrc, or the ~/.hellishrc already there) ───────
# Delegated, not inlined. This is the copy `make my_shell` did not have, so
# the sudo route installed a shell with no config at all (issue #51); one
# seeder called by both routes is what keeps them from drifting again.
# --path-dir is the other half of "install it and it works": $DEST_DIR is
# NOT necessarily on PATH (see the block the seeder writes), so without it
# the shell starts but its own name does not resolve.
"$REPO_ROOT/tools/seed_hellishrc.sh" --example "$REPO_ROOT/hellishrc.example" \
	--path-dir "$DEST_DIR" --bin "$DEST"

# ── 3. prove the binary works BEFORE anything execs it ────────────────────
#
# This is the whole safety story. Once the hook is in place, every new
# terminal runs this binary as its shell; if it is broken, terminals die on
# open and you have a genuinely annoying afternoon. So the hook is only ever
# written after the installed binary has demonstrably run a command, honoured
# an exit status, and accepted input on a pipe.
inf "smoke-testing $DEST"
smoke_st=0
quiet "$DEST" -c 'exit 42' >/dev/null 2>&1 || smoke_st=$?
[ "$smoke_st" = "42" ] || die "smoke test failed: '$DEST -c' returned $smoke_st, want 42"
smoke_st=0
printf 'exit 7\n' | quiet "$DEST" >/dev/null 2>&1 || smoke_st=$?
[ "$smoke_st" = "7" ] || die "smoke test failed: piped input returned $smoke_st, want 7"
grn "binary runs commands and reports status correctly"

# A broken rc cannot stop the shell from starting (it is sourced, not
# exec'd), so this is a warning and not a hard failure — but you want to
# hear about it now rather than wonder why your aliases are missing. The one
# checked is the one the shell reads: ~/.polishrc when it exists, else
# ~/.hellishrc (brand_pick) -- under either binary name, so a kept
# ~/.local/bin/hellish beside both rcs is checked against ~/.polishrc. Only a
# pre-3.4 binary (--version "hellish, version") reads ~/.hellishrc alone.
case "$(quiet "$DEST" --version </dev/null 2>/dev/null | head -1)" in
hellish[,\ ]*) USER_RC="$HOME/.hellishrc" ;;
*)	USER_RC="$HOME/.polishrc"
	[ -e "$USER_RC" ] || USER_RC="$HOME/.hellishrc" ;;
esac
if [ -f "$USER_RC" ]; then
	if ! quiet "$DEST" -c ". \"\$1\"" sh "$USER_RC" >/dev/null 2>&1; then
		warn "$USER_RC reported errors when sourced — check it"
	fi
fi

# ── 4. write the hook ─────────────────────────────────────────────────────
# The rc being hooked loses any 2.7.x block first: the one written below
# supersedes it. Other rc files keep theirs until uninstall -- a hook in
# ~/.bashrc may be the only reason a zsh user who execs bash ever reaches
# hellish (issue #116), and that is their arrangement to change.
#
# One block, ever: a `# >>> hellish >>>` block from a pre-3.4 install is
# REPLACED under its own marker (an older checkout's uninstaller still finds
# it), never joined by a `# >>> polish >>>` one. Two exec hooks would be
# harmless only by the luck of the first one exec'ing.
[ -e "$RC_TARGET" ] || : > "$RC_TARGET"
drop_legacy_hooks "$RC_TARGET"
if grep -qxF "$LEGACY_BEGIN" "$RC_TARGET"; then
	BEGIN_MARK="$LEGACY_BEGIN"
	END_MARK="$LEGACY_END"
fi
cp -p "$RC_TARGET" "$RC_TARGET.polish-bak"
strip_block < "$RC_TARGET.polish-bak" | trim_trailing_blanks > "$RC_TARGET.tmp"

cat >> "$RC_TARGET.tmp" <<EOF

$BEGIN_MARK
# Added by polish's user-install.sh. Everything between the markers is
# regenerated on re-install and deleted by \`make user-uninstall\` — put your
# own edits outside them.
#
# chsh(1) will not point at a shell that is missing from /etc/shells, and
# only root writes that file. So the handover happens here instead: an
# interactive shell replaces ITSELF with polish. exec means no wrapper
# process is left behind — polish owns the terminal, the pid and the exit.
#
# This block sits at the END of the file on purpose: whatever you set up
# above (PATH, nvm, proxies) is already in the environment, and exec carries
# the environment across, so polish starts with the setup you just built.
#
# Ways out, in increasing order of permanence (the HELLISH_ spellings and
# ~/.hellish-disable from before the rename work too):
#   POLISH_NO_EXEC=1 <terminal>     one session in $LOGIN_SHELL
#   touch ~/.polish-disable         every new session, until you delete it
#   make user-uninstall             remove this block for good
# And from another machine, since your passwd shell is untouched:
#   ssh $(id -un)@\$(hostname) 'touch ~/.polish-disable'

# The binary lives in a directory your login chain may not have on PATH:
# ~/.profile adds ~/.local/bin only when it already exists at login, and on a
# first install this run is what created it. The exec below uses an absolute
# path, so the shell starts either way — but without this, \`polish\` as a
# COMMAND ("polish update", "command -v polish", any tool that looks the
# shell up by name) is not found, here or inside polish. Guarded, because
# an rc gets re-sourced and an unguarded prepend stacks duplicates.
case ":\$PATH:" in
	*":$DEST_DIR:"*) ;;
	*) PATH="$DEST_DIR:\$PATH"; export PATH ;;
esac

if [ -z "\${POLISH_NO_EXEC-}\${HELLISH_NO_EXEC-}" ] \\
	&& [ -z "\${POLISH_EXECD-}\${HELLISH_EXECD-}" ] \\
	&& [ ! -e "\$HOME/.polish-disable" ] && [ ! -e "\$HOME/.hellish-disable" ] \\
	&& [ -x "$DEST" ]; then
	case \$- in
	*i*)
		# Exported BEFORE the exec, so the shell and everything under it
		# inherits the marker. That is what lets you type \`$LOGIN_SHELL\`
		# inside it and actually get $LOGIN_SHELL instead of bouncing
		# straight back here. Both spellings: a pre-3.4 hook left in another
		# rc only knows HELLISH_EXECD.
		POLISH_EXECD=1
		HELLISH_EXECD=1
		export POLISH_EXECD HELLISH_EXECD
		# So \$SHELL-respecting tools (tmux, vim's :shell, xdg-terminal)
		# spawn it too, which is the other half of "default shell".
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
grn "hooked $RC_TARGET (backup: $RC_TARGET.polish-bak)"

# ── done ──────────────────────────────────────────────────────────────────
printf '
  \033[1;37mpolish is now your default shell.\033[0m Open a new terminal — no logout needed.
  In THIS one, right now:   \033[1;36mexec %s\033[0m

  Your config lives in \033[1;36m%s\033[0m (sourced by every interactive
  shell, never by scripts or -c). Shell functions do not survive exec, so
  anything you had as a %s function — nvm, rbenv, conda — belongs in there
  too.

  Undo everything:          \033[1;36mmake user-uninstall\033[0m

' "$DEST" "$USER_RC" "$LOGIN_SHELL" >&2

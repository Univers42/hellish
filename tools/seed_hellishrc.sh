#!/bin/sh
# ============================================================================
# tools/seed_hellishrc.sh -- put hellishrc.example at ~/.polishrc (or, on a
# machine that already has one, ~/.hellishrc), once, and keep the one block in
# it that the INSTALLER owns: the PATH entry for the directory the binary
# actually landed in.
#
# Why this is its own file. The seeding used to live inline in
# user-install.sh, which meant it only happened on the no-sudo route. The
# sudo route -- `make my_shell`, install to /usr/bin and chsh -- installed a
# binary and stopped, so a user who took it got a shell with no config at
# all: no EDITOR, no aliases, no PS1, and no hint that a config file was
# ever meant to exist. That is the first line of issue #51. One seeder,
# called by both routes, is the only shape that cannot drift again.
#
# The one rule that matters: an EXISTING rc is never touched. It is the
# user's file, installers get re-run, and eating an edited config is far
# worse than skipping a template. --path-dir does not break that rule: it
# rewrites ONLY the text between its own markers, the same way
# user-install.sh manages its block in your login shell's rc.
#
# Which rc. The shell was renamed hellish -> polish in 3.4.0 and reads
# ~/.polishrc when it exists or when ~/.hellishrc does not, else ~/.hellishrc
# (brand_pick, src/brand/brand.c). This seeder makes the same choice, so a
# machine that has a ~/.hellishrc never grows a ~/.polishrc beside it: the
# new file would win, and the user's real config would silently stop
# loading. Same rule for ~/.config/polish vs ~/.config/hellish. A pre-3.4
# binary (--bin answering "hellish, version") only knows the old names, so
# it always gets them.
#
#   tools/seed_hellishrc.sh                      seed $HOME/.polishrc
#   tools/seed_hellishrc.sh --home DIR           seed DIR/.polishrc
#   tools/seed_hellishrc.sh --example FILE       use FILE as the template
#   tools/seed_hellishrc.sh --path-dir DIR       ...and keep DIR on PATH
#   tools/seed_hellishrc.sh --bin PATH           the binary that will read it
#   tools/seed_hellishrc.sh --strip-path         drop the PATH block again
#
# Always exits 0 unless it was asked to do something impossible: a missing
# template is a warning, because an install must not fail over a doc file.
# ============================================================================
set -u

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET_HOME="${HOME:-}"
EXAMPLE="$REPO_ROOT/hellishrc.example"
PATH_DIR=""
BIN=""
STRIP_PATH=0

while [ $# -gt 0 ]; do
	case "$1" in
	--home) TARGET_HOME="${2:-}"; shift 2 ;;
	--example) EXAMPLE="${2:-}"; shift 2 ;;
	--path-dir) PATH_DIR="${2:-}"; shift 2 ;;
	--bin) BIN="${2:-}"; shift 2 ;;
	--strip-path) STRIP_PATH=1; shift ;;
	-h|--help) sed -n '3,41p' "$0"; exit 0 ;;
	*) echo "seed_hellishrc: unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$TARGET_HOME" ]; then
	echo "seed_hellishrc: no \$HOME and no --home; nothing to seed" >&2
	exit 2
fi

# Every child we run gets both spellings of the quiet knobs: a 3.4.0+ binary
# reads POLISH_* first, an older one only knows HELLISH_*.
quiet() {
	POLISH_NO_BANNER=1 HELLISH_NO_BANNER=1 \
	POLISH_NO_UPDATE_CHECK=1 HELLISH_NO_UPDATE_CHECK=1 \
	POLISH_NO_ANIM=1 HELLISH_NO_ANIM=1 "$@"
}

# The binary we seed for, if we were told: "hellish" for a pre-3.4 build
# (its --version starts "hellish, version"), else "polish". No --bin, or a
# binary that cannot answer, means the current one.
BRAND="polish"
if [ -n "$BIN" ] && [ -x "$BIN" ]; then
	case "$(quiet "$BIN" --version </dev/null 2>/dev/null | head -1)" in
	hellish[,\ ]*) BRAND="hellish" ;;
	esac
fi

# pick NEW LEGACY -> the path the shell will actually read (see the header).
pick() {
	if [ "$BRAND" = "hellish" ]; then
		printf '%s\n' "$2"
	elif [ -e "$1" ] || [ ! -e "$2" ]; then
		printf '%s\n' "$1"
	else
		printf '%s\n' "$2"
	fi
}

RC="$(pick "$TARGET_HOME/.polishrc" "$TARGET_HOME/.hellishrc")"

# The interpreter the copied modules' shebang names. share/ ships
# `#!/usr/bin/env polish`; a machine with no `polish` command -- a pre-3.4
# binary, or an old install the 3.3 updater upgraded in place, which keeps
# only the hellish name -- gets `hellish` instead, so a module run as a
# script still finds its shell. A fresh install has polish, and sees no
# hellish name anywhere.
has_cmd() {
	for _d in "$PATH_DIR" "${BIN%/*}"; do
		[ -n "$_d" ] && [ -x "$_d/$1" ] && return 0
	done
	command -v "$1" >/dev/null 2>&1
}
SHEBANG="polish"
if [ "$BRAND" = "hellish" ] || { ! has_cmd polish && has_cmd hellish; }; then
	SHEBANG="hellish"
fi

# copy_module SRC DEST: cp, with line 1 naming SHEBANG.
copy_module() {
	sed "1s|^#!/usr/bin/env polish\$|#!/usr/bin/env $SHEBANG|" "$1" > "$2"
}

# Where the config tree goes. XDG_CONFIG_HOME is honoured only when it
# actually lives UNDER the home being seeded.
#
# Anything looser seeds the wrong machine. Comparing TARGET_HOME to $HOME is
# not enough: every test drives this by overriding HOME, which makes the two
# equal while the ambient XDG_CONFIG_HOME still points at the developer's own
# ~/.config. The first version of this did exactly that and created
# ~/.config/hellish on my box while reporting that it had seeded a temp dir.
case "${XDG_CONFIG_HOME:-}" in
"$TARGET_HOME"/*) XDG_BASE="$XDG_CONFIG_HOME" ;;
*) XDG_BASE="$TARGET_HOME/.config" ;;
esac
XDG="$(pick "$XDG_BASE/polish" "$XDG_BASE/hellish")"

# The PATH block's markers follow the rc's own name, so a pre-3.4 uninstaller
# still finds its block in ~/.hellishrc. Both spellings are stripped always:
# one rc must never carry two blocks.
LEGACY_PATH_BEGIN='# >>> hellish path >>>'
LEGACY_PATH_END='# <<< hellish path <<<'
case "$RC" in
*/.hellishrc) PATH_BEGIN="$LEGACY_PATH_BEGIN"; PATH_END="$LEGACY_PATH_END" ;;
*) PATH_BEGIN='# >>> polish path >>>'; PATH_END='# <<< polish path <<<' ;;
esac

# ── seeding ────────────────────────────────────────────────────────────────
seed_rc() {
	if [ -e "$RC" ]; then
		printf '  \033[1;34mi\033[0m  %s already exists — left untouched\n' "$RC"
		return 0
	fi
	# A pre-3.4 --bin picks ~/.hellishrc even when ~/.polishrc exists. A
	# second rc would split the config: the 3.4 upgrade reads only polishrc.
	if [ -e "$TARGET_HOME/.polishrc" ]; then
		printf '  \033[1;34mi\033[0m  %s exists — no %s seeded beside it\n' \
			"$TARGET_HOME/.polishrc" "$RC"
		return 0
	fi

	if [ ! -f "$EXAMPLE" ]; then
		printf '  \033[1;33m!\033[0m  %s not found — skipping %s\n' \
			"$EXAMPLE" "$RC" >&2
		return 0
	fi

	# Copy to a temp name in the same directory and rename, so an interrupted
	# copy can never leave a half-written rc that the next shell then sources.
	cp "$EXAMPLE" "$RC.new" || return 1
	mv -f "$RC.new" "$RC" || return 1
	printf '  \033[1;32m✓\033[0m  seeded %s from %s\n' "$RC" "$(basename "$EXAMPLE")"
}


# -- the default prompt, as a FILE -----------------------------------------
# Issue #74: "this prompt should exist in configuration so we can retouch it
# and not in binary only". The shell keeps a built-in fallback for a machine
# with no config at all, but the prompt you actually get is this file, and
# editing it is the supported way to change it -- no rebuild, and no hunting
# for the escape sequence in C.
#
# Deliberately plain. The built-in theme is a two-row box with git, timing
# and job badges, which is a lot to meet on first launch; #74 asks for "a
# normal prompt without nothing, just maybe the update notification". So:
# three coloured segments (user, host, cwd) in the shape zsh users already
# expect, \U for a pending release, nothing else.
#
# Never overwritten: like the rc, an existing file is the user's.
seed_prompt() {
	if [ -e "$XDG/rc.d/30-prompt.hsh" ]; then
		printf '  \033[1;34mi\033[0m  %s already exists -- left untouched\n' \
			"$XDG/rc.d/30-prompt.hsh"
		return 0
	fi
	mkdir -p "$XDG/rc.d" "$XDG/plugins" 2>/dev/null || return 0
	cat > "$XDG/rc.d/30-prompt.hsh" <<'PROMPT_EOF'
# The prompt. Yours to edit -- polish reads this file, it is not compiled in.
#
#   \u user   \h host   \w cwd (~)   \W basename   \$ $ or #
#   \U  a pending release, invisible otherwise
#   \g git branch   \S failure badge   \p duration   \J jobs
#
# Single quotes matter: in double quotes $? and friends expand ONCE, when
# this line runs, and are then frozen forever.
PS1='\[\e[48;5;24;38;5;255m\] \u \[\e[0m\]\[\e[48;5;60;38;5;255m\] \h \[\e[0m\]\[\e[48;5;238;38;5;252m\] \w \[\e[0m\]\U \$ '
PS2='\[\e[38;5;245m\]> \[\e[0m\]'

# The full built-in theme (git, timing, jobs, two rows) is one line away:
# PS1='\B'
#
# Or the zsh spelling, if you prefer it -- set PROMPT instead of PS1:
# PROMPT='%F{green}%n@%m%f %~ %# '
PROMPT_EOF
	printf '  \033[1;32mok\033[0m  seeded %s\n' "$XDG/rc.d/30-prompt.hsh"
}

# ── the prompt themes and the `prompt` switcher ────────────────────────────
#
# 29 themes plus one rc.d file defining `prompt`. Copied, not symlinked: they
# are yours once they are there, and an upgrade must not silently rewrite a
# theme you edited.
#
# Per-file never-clobber, not all-or-nothing. A user who edited two themes
# still gets the twenty-seven new ones on the next upgrade, and the two they
# touched are left exactly as they are -- an all-or-nothing check would mean
# one edit freezes the whole set forever.
#
# The switcher itself IS overwritten, and that is the one deliberate
# exception: it is code rather than configuration, a stale copy would not
# know about a new subcommand, and anyone who wants their own can define
# `prompt` after it loads -- the last definition wins.
seed_themes() {
	_src="$(dirname "$0")/../share"
	[ -d "$_src/themes" ] || return 0
	mkdir -p "$XDG/themes" "$XDG/rc.d" 2>/dev/null || return 0
	_new=0
	_kept=0
	for _f in "$_src"/themes/*.hsh "$_src"/themes/README.md; do
		[ -e "$_f" ] || continue
		if [ -e "$XDG/themes/$(basename "$_f")" ]; then
			_kept=$((_kept + 1))
		else
			copy_module "$_f" "$XDG/themes/$(basename "$_f")" 2>/dev/null \
				&& _new=$((_new + 1))
		fi
	done
	copy_module "$_src/rc.d/40-prompt-switch.hsh" \
		"$XDG/rc.d/40-prompt-switch.hsh" 2>/dev/null
	if [ "$_kept" -gt 0 ]; then
		printf '  \033[1;32mok\033[0m  %s themes (%s new, %s of yours left alone)\n' \
			"$XDG/themes" "$_new" "$_kept"
	else
		printf '  \033[1;32mok\033[0m  seeded %s themes in %s\n' \
			"$_new" "$XDG/themes"
	fi
	printf '        try:  prompt          (list)   prompt preview   (see them all)\n'
}

# ── the managed PATH block ─────────────────────────────────────────────────
# Delete an existing block from stdin, under either marker spelling. A
# half-removed block (someone deleted one marker by hand) degrades to "leave
# it alone" rather than eating the rest of the file.
strip_path_block() {
	awk -v b="$PATH_BEGIN" -v e="$PATH_END" \
		-v lb="$LEGACY_PATH_BEGIN" -v le="$LEGACY_PATH_END" '
		$0 == e || $0 == le { skip = 0; next }
		$0 == b || $0 == lb { skip = 1 }
		skip    { next }
		        { print }
	'
}

# Drop trailing blank lines. We insert a blank line ahead of the block, so
# without this an install/uninstall/install cycle would slowly grow a gap at
# the end of the file. Interior blank lines are preserved.
trim_trailing_blanks() {
	awk 'BEGIN { n = 0 }
	     /^[[:space:]]*$/ { n++; next }
	     { while (n-- > 0) print ""; n = 0; print }'
}

write_path_block() {
	_dir="$1"
	[ -e "$RC" ] || : > "$RC"
	strip_path_block < "$RC" | trim_trailing_blanks > "$RC.new" || return 1

	cat >> "$RC.new" <<EOF

$PATH_BEGIN
# Added by polish's installer. Everything between the markers is
# regenerated on re-install and removed by \`make user-uninstall\` — put your
# own edits outside them.
#
# Why this is here at all. \`make user-install\` puts the binary somewhere
# your login chain may well NOT have on PATH. Debian/Ubuntu's stock
# ~/.profile adds ~/.local/bin only when that directory already exists at
# login — and on a first install this is the run that created it. A
# user-installed polish is also exec'd from an interactive rc, so it is not
# a login shell and never reads /etc/profile or ~/.profile itself.
#
# The shell still STARTS without this (the rc hook execs an absolute path),
# which is exactly why the gap was easy to miss: what broke was the NAME.
# \`polish\`, \`polish update\`, \`command -v polish\` and every tool that
# looks the shell up by name answered "command not found" on a freshly
# installed machine. See tests/user_install_path_test.py.
#
# The case guard is load-bearing: this file is re-sourced by every nested
# interactive shell, and an unguarded prepend stacks a fresh copy each
# time until PATH is mostly duplicates.
case ":\$PATH:" in
	*":$_dir:"*) ;;
	*) PATH="$_dir:\$PATH"; export PATH ;;
esac
$PATH_END
EOF

	# Refuse to leave behind an rc that will not parse: this file is sourced
	# by every interactive hellish, and this script is the one that wrote it.
	#
	# Asked of the shell that READS the file. This used to be `sh -n`, and
	# /bin/sh is dash on Debian and Ubuntu: it rejects `HX_LOADED=()` -- the
	# plugin framework's own loader, bash syntax hellish accepts -- so every
	# re-run over a framework rc refused the block, returned 1, and the
	# caller's `set -e` turned "left untouched" into an install that stopped
	# right there: binary copied, nothing hooked, no framework question.
	#
	# And only OUR change can be refused. When the file did not parse before
	# the block went in either, that is the user's rc, not this script's
	# doing; hellish reports rc errors and carries on, so the block is added
	# and the state of the rest is reported rather than made a blocker.
	if ! rc_parses "$RC.new"; then
		if rc_parses "$RC"; then
			rm -f "$RC.new"
			printf '  \033[1;33m!\033[0m  generated %s would not parse — left untouched\n' \
				"$RC" >&2
			return 1
		fi
		printf '  \033[1;33m!\033[0m  %s does not parse as it is; the PATH block is added anyway\n' \
			"$RC" >&2
	fi
	mv -f "$RC.new" "$RC" || return 1
	printf '  \033[1;32m✓\033[0m  %s puts %s on PATH\n' "$RC" "$_dir"
}

# rc_parses FILE -> 0 when the shell that will source FILE accepts it: --bin,
# else the polish (or pre-3.4 hellish) this install just put in place, else
# bash (whose syntax the shell reads), else, for want of anything better, sh.
rc_parses() {
	_sh="$BIN"
	if [ -z "$_sh" ] && [ -n "$PATH_DIR" ]; then
		for _sh in "$PATH_DIR/polish" "$PATH_DIR/hellish" ""; do
			[ -x "$_sh" ] && break
		done
	fi
	if [ -n "$_sh" ] && [ -x "$_sh" ]; then
		quiet "$_sh" -n "$1" >/dev/null 2>&1
	elif command -v bash >/dev/null 2>&1; then
		bash -n "$1" >/dev/null 2>&1
	else
		sh -n "$1" >/dev/null 2>&1
	fi
}

# Both rc names: an uninstall must not depend on which one this machine has.
remove_path_block() {
	for _rc in "$TARGET_HOME/.polishrc" "$TARGET_HOME/.hellishrc"; do
		[ -f "$_rc" ] || continue
		grep -qxF -e "$PATH_BEGIN" -e "$LEGACY_PATH_BEGIN" "$_rc" 2>/dev/null \
			|| continue
		strip_path_block < "$_rc" | trim_trailing_blanks > "$_rc.new" \
			|| return 1
		mv -f "$_rc.new" "$_rc" || return 1
		printf '  \033[1;32m✓\033[0m  removed the PATH block from %s\n' "$_rc"
	done
}

if [ "$STRIP_PATH" = "1" ]; then
	remove_path_block || exit 1
	exit 0
fi

seed_rc || exit 1
seed_prompt || exit 1
seed_themes || exit 1
# A refused block is a warning, not a failed install: the shell starts
# without it (the hook execs an absolute path), and the message above says
# what is missing. Exiting 1 here killed the caller under set -e.
if [ -n "$PATH_DIR" ]; then
	write_path_block "$PATH_DIR" || true
fi
exit 0

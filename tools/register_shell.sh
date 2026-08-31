#!/bin/sh
# ============================================================================
# tools/register_shell.sh -- install hellish system-wide and make it the login
# shell, safely. This is the second half of `make my_shell`.
#
# WHY THIS EXISTS
# ---------------
# It used to be vendor/scripts/register_shell.sh -- fourteen lines, in a
# DIFFERENT repository (a submodule), with no preflight, no privilege
# handling, no error checking and no test coverage:
#
#     if ! grep -qx "$SHELL_PATH" /etc/shells; then
#             echo "$SHELL_PATH" | sudo tee -a /etc/shells > /dev/null
#     fi
#     echo "Setting default shell for $USER"
#     chsh -s "$SHELL_PATH"
#
# Every one of these was reproducible in a clean container:
#
#   * bare `chsh` with no user operand prompts for a PASSWORD. Under make,
#     stdin is not a tty, so PAM fails and `make my_shell` died with
#     "chsh: PAM: Authentication failure" -- AFTER installing the binary and
#     editing /etc/shells. A partial install, and the "how to use it now"
#     instructions never printed. `chsh` run as root does not prompt, so the
#     fix is to escalate the way the /etc/shells write already did.
#   * $USER is not exported in a container or a non-login shell, so the
#     message read "Setting default shell for " and, under `sudo make`, the
#     bare chsh would have targeted ROOT rather than the human.
#   * a missing /etc/shells made `grep` fail, which `!` turned into "not
#     present", so `tee -a` CREATED the file containing our entry and nothing
#     else. A one-shell whitelist makes chsh treat every other account on the
#     box as restricted and refuse to change it.
#   * no chsh (Alpine without `shadow`) exited 127; no sudo exited 127; and
#     both did so only after a multi-minute rebuild had already finished.
#   * nothing ever checked that the binary RUNS. chsh happily accepts a shell
#     that exits 1 immediately, and a broken login shell locks you out of ssh
#     and every tty -- only root can undo it. user-install.sh, the sibling
#     route, has proven the binary before writing its hook since day one; the
#     route that does the strictly more dangerous thing proved nothing.
#
# So: preflight before anything is built, smoke-test before anything is
# exec'd, escalate once and explicitly, verify afterwards. The same order
# user-install.sh uses, because it is the order that cannot lock you out.
#
#   tools/register_shell.sh --preflight        can this machine do it at all?
#   tools/register_shell.sh --bin build/bin/hellish     install and register
#   tools/register_shell.sh --dry-run --bin X  say what it would do, do nothing
#   tools/register_shell.sh --uninstall        put the previous shell back
#   tools/register_shell.sh --purge            ...and the config too
#   tools/register_shell.sh --doctor           what is installed, and will it update?
# ============================================================================
set -eu

PROG="$(basename "$0")"

BIN_SRC=""
DEST="${DEST:-/usr/bin/hellish}"
TARGET_USER=""
ACTION="register"
DRY_RUN=0
PURGE=0

# ── plumbing (same vocabulary as user-install.sh) ──────────────────────────
red()  { printf '\033[1;31m%s\033[0m\n' "$*" >&2; }
grn()  { printf '  \033[1;32m✓\033[0m %s\n' "$*" >&2; }
inf()  { printf '  \033[1;36m▸\033[0m %s\n' "$*" >&2; }
warn() { printf '  \033[1;33m!\033[0m %s\n' "$*" >&2; }
die()  { red "$PROG: $*"; exit 1; }

usage() {
	cat >&2 <<EOF
usage: $PROG [--preflight|--uninstall|--doctor] [--bin PATH] [--dest PATH] [--user NAME]

  --preflight    check this machine CAN install a login shell, change nothing
  --bin PATH     the freshly built binary to install
  --dest PATH    where it goes            (default: $DEST)
  --user NAME    whose login shell to set (default: the invoking human)
  --dry-run      print every privileged action instead of running it
  --uninstall    restore the login shell, remove the binary and /etc/shells entry
  --purge        ...and delete ~/.hellishrc, ~/.config/hellish, ~/.cache/hellish
  --doctor       report what is installed and whether updating will work
EOF
	exit 2
}

while [ $# -gt 0 ]; do
	case "$1" in
	--preflight) ACTION="preflight" ;;
	--uninstall) ACTION="uninstall" ;;
	--doctor)    ACTION="doctor" ;;
	--purge)     ACTION="uninstall"; PURGE=1 ;;
	--dry-run)   DRY_RUN=1 ;;
	--bin)   shift; [ $# -gt 0 ] || usage; BIN_SRC="$1" ;;
	--dest)  shift; [ $# -gt 0 ] || usage; DEST="$1" ;;
	--user)  shift; [ $# -gt 0 ] || usage; TARGET_USER="$1" ;;
	-h|--help) usage ;;
	*) red "$PROG: unknown argument '$1'"; usage ;;
	esac
	shift
done

# ── who are we actually installing FOR ────────────────────────────────────
# NOT $USER: it is not exported in containers, in `su` sessions or in any
# non-login shell, and an empty value silently retargets chsh at the real
# uid. SUDO_USER first, so `sudo make my_shell` still sets the HUMAN's login
# shell rather than root's -- that mistake is invisible until the next login.
if [ -z "$TARGET_USER" ]; then
	TARGET_USER="${SUDO_USER:-$(id -un)}"
fi

# ── how we become root, decided ONCE ──────────────────────────────────────
# The old script sudo'd the /etc/shells write but not the chsh, which is the
# whole bug. One answer, used for both.
ROOT_CMD=""
if [ "$(id -u)" = "0" ]; then
	ROOT_CMD=""
elif command -v sudo >/dev/null 2>&1; then
	ROOT_CMD="sudo"
fi

have_root() { [ "$(id -u)" = "0" ] || command -v sudo >/dev/null 2>&1; }

as_root() {
	if [ "$DRY_RUN" = "1" ]; then
		printf '  would run: %s %s\n' "${ROOT_CMD:-(as root)}" "$*" >&2
		return 0
	fi
	$ROOT_CMD "$@"
}

# ── preflight: everything that can say "not on this machine" ──────────────
# Runs BEFORE the rebuild. The old script's failures all landed after a
# multi-minute build had completed, which is the most expensive possible
# moment to discover that chsh is not installed.
preflight() {
	rc=0

	if ! have_root; then
		red "$PROG: need root to install $DEST, and there is no sudo here"
		red "  you are '$TARGET_USER' (uid $(id -u)) and sudo is not installed."
		red "  Use the no-root route instead:  make user-install"
		rc=1
	fi

	if ! command -v chsh >/dev/null 2>&1; then
		red "$PROG: chsh(1) is not installed"
		red "  Debian/Ubuntu: apt install passwd   ·   Alpine: apk add shadow"
		red "  Or skip chsh entirely:  make user-install"
		rc=1
	fi

	if ! id -- "$TARGET_USER" >/dev/null 2>&1; then
		red "$PROG: no such user '$TARGET_USER'"
		rc=1
	elif ! getent passwd -- "$TARGET_USER" >/dev/null 2>&1 \
		&& ! grep -q "^$TARGET_USER:" /etc/passwd 2>/dev/null; then
		# chsh edits /etc/passwd. An LDAP/SSSD-only account has no local
		# entry and chsh cannot rewrite it -- better said now than after
		# the binary is in /usr/bin.
		warn "'$TARGET_USER' has no local /etc/passwd entry"
		warn "chsh may refuse it (LDAP/SSSD accounts are managed elsewhere)"
	fi

	# $HOME is what tools/seed_hellishrc.sh needs; it exits 2 without one and
	# would fail the target after the install.
	if [ -z "${HOME:-}" ]; then
		red "$PROG: \$HOME is unset — the config seeder cannot run"
		rc=1
	fi

	[ "$rc" = "0" ] || exit 1

	# Not a failure, but a sudo password prompt appearing three minutes into
	# a build looks like a hang. Say it now.
	if [ -n "$ROOT_CMD" ] && ! sudo -n true 2>/dev/null; then
		inf "sudo will ask for your password during the install"
	fi
	grn "preflight ok — can install $DEST and set the shell for $TARGET_USER"
}

# ── prove the binary RUNS before anything can exec it ─────────────────────
# The check user-install.sh has always had and this route never did. Once
# chsh lands, this binary IS the login shell: if it is broken, ssh and every
# tty die on connect and only root can put it back.
smoke_test() {
	_bin="$1"
	inf "smoke-testing $_bin"
	_st=0
	HELLISH_NO_BANNER=1 HELLISH_NO_ANIM=1 HELLISH_NO_UPDATE_CHECK=1 \
		"$_bin" -c 'exit 42' >/dev/null 2>&1 || _st=$?
	[ "$_st" = "42" ] \
		|| die "smoke test failed: '$_bin -c' returned $_st, want 42 — NOT registering it as your login shell"
	_st=0
	printf 'exit 7\n' | HELLISH_NO_BANNER=1 HELLISH_NO_ANIM=1 \
		HELLISH_NO_UPDATE_CHECK=1 "$_bin" >/dev/null 2>&1 || _st=$?
	[ "$_st" = "7" ] \
		|| die "smoke test failed: piped input returned $_st, want 7 — NOT registering it as your login shell"
	grn "binary runs commands and reports status correctly"
}

# ── /etc/shells ───────────────────────────────────────────────────────────
# chsh refuses any shell not listed here, so this has to happen. The old
# version got the MISSING-file case backwards: grep failed, `!` read that as
# "absent", and the append created a file whose only entry was ours.
seed_etc_shells() {
	warn "/etc/shells does not exist — seeding it from the shells in use"
	_tmp="$(mktemp)" || die "mktemp failed"
	# nologin/false/sync are what a LOCKED account's passwd entry holds, and
	# /etc/shells means "valid login shell". Copying them in would tell
	# chsh -- and everything else that consults this file to decide whether
	# an account may log in -- that every locked system account is a real
	# one. Take the real shells only.
	{ getent passwd 2>/dev/null || cat /etc/passwd 2>/dev/null; } \
		| awk -F: '$7 ~ /^\// { print $7 }' \
		| grep -vE '/(nologin|false|true|sync)$' > "$_tmp" || true
	echo /bin/sh >> "$_tmp"
	{
		echo '# /etc/shells: valid login shells'
		sort -u "$_tmp" | while IFS= read -r _sh; do
			if [ -x "$_sh" ]; then printf '%s\n' "$_sh"; fi
		done
	} > "$_tmp.out"
	rm -f "$_tmp"
	if [ "$DRY_RUN" = "1" ]; then
		printf '  would create /etc/shells with:\n' >&2
		sed 's/^/    /' "$_tmp.out" >&2
		rm -f "$_tmp.out"
		return 0
	fi
	$ROOT_CMD tee /etc/shells < "$_tmp.out" >/dev/null
	rm -f "$_tmp.out"
}

ensure_in_etc_shells() {
	[ -f /etc/shells ] || seed_etc_shells
	# -F, and a real -f guard above, so a missing file can never be read as
	# "your shell is not listed yet".
	if [ -f /etc/shells ] && grep -qxF -- "$DEST" /etc/shells; then
		grn "$DEST is already listed in /etc/shells"
		return 0
	fi
	inf "adding $DEST to /etc/shells"
	if [ "$DRY_RUN" = "1" ]; then
		printf '  would append %s to /etc/shells\n' "$DEST" >&2
		return 0
	fi
	printf '%s\n' "$DEST" | $ROOT_CMD tee -a /etc/shells >/dev/null
}

# Read a user's login shell out of the passwd db.
#
# NOT `getent ... | awk ... || awk /etc/passwd`: in a pipeline it is AWK's
# status that decides, and awk on empty input succeeds. So the fallback could
# never fire, and on a box with no getent (busybox/Alpine without musl-utils)
# this returned empty -- which the verification below would have reported as
# "chsh reported success but the shell is still ''" on a install that had in
# fact worked. Pick the source first, parse second.
current_shell_of() {
	_u="$1"
	_line=""
	if command -v getent >/dev/null 2>&1; then
		_line="$(getent passwd -- "$_u" 2>/dev/null || true)"
	fi
	if [ -z "$_line" ] && [ -r /etc/passwd ]; then
		_line="$(awk -F: -v u="$_u" '$1 == u { print; exit }' /etc/passwd \
			2>/dev/null || true)"
	fi
	printf '%s' "$_line" | awk -F: '{ print $7 }'
}

# ── the actual registration ───────────────────────────────────────────────
register() {
	[ -n "$BIN_SRC" ] || die "--bin is required (which binary to install?)"
	[ -f "$BIN_SRC" ] || die "no binary at $BIN_SRC — did the build succeed?"
	[ -x "$BIN_SRC" ] || die "$BIN_SRC is not executable"

	preflight

	# Before /usr/bin is touched at all.
	smoke_test "$BIN_SRC"

	inf "installing $BIN_SRC -> $DEST"
	as_root install -m 755 "$BIN_SRC" "$DEST"
	grn "installed $DEST"

	# ...and again at the destination, because that is the path chsh writes
	# into the passwd entry. A dest on a noexec mount, or a truncated copy,
	# is caught here rather than at your next login.
	[ "$DRY_RUN" = "1" ] || smoke_test "$DEST"

	ensure_in_etc_shells

	_was="$(current_shell_of "$TARGET_USER")"
	if [ "$_was" = "$DEST" ]; then
		grn "$TARGET_USER's login shell is already $DEST"
		return 0
	fi
	# Remember the shell we replaced, so --uninstall is not a guess.
	if [ "$DRY_RUN" != "1" ] && [ -n "$_was" ] && [ -n "${HOME:-}" ]; then
		printf '%s\n' "$_was" > "$HOME/.hellish-previous-shell" 2>/dev/null \
			|| true
	fi

	inf "setting $TARGET_USER's login shell to $DEST"
	# The fix, in one line: run chsh AS ROOT and name the user explicitly.
	# As root it does not prompt, so it works under make; naming the user
	# means `sudo make my_shell` cannot retarget root by accident.
	if ! as_root chsh -s "$DEST" -- "$TARGET_USER"; then
		red "$PROG: chsh failed for '$TARGET_USER'"
		red "  $DEST is installed and listed in /etc/shells; only the"
		red "  passwd entry did not change. Retry with:"
		red "      sudo chsh -s $DEST $TARGET_USER"
		red "  or use the no-chsh route:  make user-install"
		exit 1
	fi

	# Verify, rather than trust chsh's exit status.
	if [ "$DRY_RUN" != "1" ]; then
		_now="$(current_shell_of "$TARGET_USER")"
		[ "$_now" = "$DEST" ] \
			|| die "chsh reported success but $TARGET_USER's shell is still '$_now'"
	fi
	grn "$TARGET_USER's login shell is now $DEST"

	# Never fail the install over the report -- the install worked, and these
	# are warnings about the machine, not about what we just did.
	[ "$DRY_RUN" = "1" ] || doctor || true
}

# ── doctor: the two things that go wrong AFTER a successful install ───────
#
# Both were reported as "the path is wrong" and neither is $PATH being wrong.
#
# 1. A SHADOWED binary. `make user-install` puts hellish in ~/.local/bin,
#    which Ubuntu's ~/.profile puts BEFORE /usr/bin. Run user-install once and
#    my_shell later and there are two hellishes: the one you type is the old
#    user copy, the one chsh logs you into is the new system one. `update`
#    then replaces whichever binary is RUNNING, so the other stays stale for
#    ever and every symptom looks like "the update did nothing". Two copies is
#    not itself an error -- it is only ever a surprise, so say it out loud.
#
# 2. An install whose directory needs elevation. That is normal for /usr/bin
#    and it is what my_shell is for, but the user should learn it here rather
#    than from a password prompt in the middle of their first update.
doctor() {
	_rc=0
	inf "hellish install report"

	if [ ! -e "$DEST" ]; then
		warn "nothing installed at $DEST"
		_rc=1
	else
		# Ask the thing at DEST what it is. An answer we cannot read is NOT a
		# clean bill of health -- the first version of this reported "no
		# problems found" next to a literal "(?)" on a box where $DEST was a
		# hand-written bash wrapper execing a hellish.real beside it. A report
		# that says all-clear on a machine it did not understand is worse than
		# no report, so an unreadable version is a finding.
		_ver="$( "$DEST" -c 'update --version' 2>/dev/null | head -1 )"
		case "$_ver" in
		hellish\ *) grn "$DEST ($_ver)" ;;
		*)
			warn "$DEST does not report a hellish version"
			[ -n "$_ver" ] && warn "  it answered: $_ver"
			# The shape this takes in practice: something at the canonical
			# path that is not the shell -- a wrapper, a symlink to another
			# shell, a stale stub. `update` replaces the binary that is
			# RUNNING (/proc/self/exe), which in that arrangement is the real
			# one beside it, so the two can drift apart silently.
			if ! head -c 4 "$DEST" 2>/dev/null | grep -q 'ELF'; then
				warn "  it is not an ELF binary:  $(head -1 "$DEST" 2>/dev/null)"
				[ -e "$DEST.real" ] && warn "  ...and $DEST.real exists" \
					&& warn "  so \`update\` will replace $DEST.real, not $DEST"
			fi
			_rc=1
			;;
		esac
	fi

	# Every hellish PATH can reach, in PATH order. `command -v` gives only the
	# winner, which is exactly the copy that hides the problem.
	#
	# Deduplicated by RESOLVED path, not by the name we walked to. /bin is a
	# symlink to /usr/bin on every merged-/usr distro, so a single install
	# shows up under both and the naive list reports "installed twice" on a
	# machine with one copy -- a warning that is wrong is worse than none.
	_found=""
	_seen=""
	_ifs="$IFS"; IFS=:
	for _d in $PATH; do
		[ -n "$_d" ] || _d=.
		if [ -x "$_d/hellish" ]; then
			_real="$(readlink -f "$_d/hellish" 2>/dev/null || echo "$_d/hellish")"
			case " $_seen " in
			*" $_real "*) ;;
			*) _seen="$_seen $_real"; _found="$_found $_d/hellish" ;;
			esac
		fi
	done
	IFS="$_ifs"
	set -- $_found
	if [ $# -eq 0 ]; then
		red "$PROG: no hellish on your PATH at all"
		red "  installed at $DEST, but PATH cannot reach it."
		red "  PATH=$PATH"
		_rc=1
	elif [ $# -gt 1 ]; then
		warn "hellish exists $# times on PATH; you are running the FIRST:"
		for _p in "$@"; do
			printf '      %s  (%s)\n' "$_p" \
				"$( "$_p" -c 'update --version' 2>/dev/null || echo '?' )" >&2
		done
		warn "an \`update\` only replaces the one that is RUNNING, so the"
		warn "others stay behind and look like an update that did nothing."
		[ "$1" = "$DEST" ] || warn "remove $1, or run: hash -r"
		_rc=1
	elif [ "$1" != "$DEST" ]; then
		warn "PATH finds $1, but this installs to $DEST"
		_rc=1
	else
		grn "PATH finds exactly one hellish, and it is $DEST"
	fi

	# What the next update will have to do. update_needs_sudo() asks the same
	# question the same way: write access to the DIRECTORY, because replacing
	# a binary is a rename inside it.
	_dir="$(dirname "$DEST")"
	if [ -w "$_dir" ]; then
		grn "$_dir is writable — updates need no password"
	elif have_root; then
		inf "$_dir is root's — \`update\` will ask for your sudo password"
		inf "  and run: sudo install -m 755 <verified download> $DEST"
	else
		red "$PROG: $_dir is not writable and there is no sudo here"
		red "  updates cannot install. Use: make user-install"
		_rc=1
	fi

	[ "$_rc" = "0" ] && grn "no problems found"
	return $_rc
}

# Put the machine back the way it was.
#
# ORDER IS LOAD-BEARING: the login shell is restored FIRST, and the binary is
# only removed once the passwd entry no longer points at it. The other order
# leaves a window -- and, if chsh then fails, a permanent state -- where the
# account's login shell is a path that does not exist. That locks the user out
# of ssh and every tty, and only root can undo it. So a failed chsh aborts
# before anything is deleted.
#
# --purge also takes the config, which plain --uninstall deliberately keeps:
# ~/.hellishrc is the user's own work, and reinstalling to test something is
# not a reason to throw it away. `make my_shell VERSION=…` uses --purge so
# each reinstall starts from a genuinely clean machine.
uninstall() {
	_prev=""
	[ -n "${HOME:-}" ] && [ -f "$HOME/.hellish-previous-shell" ] \
		&& _prev="$(cat "$HOME/.hellish-previous-shell")"
	[ -n "$_prev" ] || _prev="/bin/sh"
	[ -x "$_prev" ] || _prev="/bin/sh"

	_now="$(current_shell_of "$TARGET_USER")"
	if [ "$_now" = "$DEST" ]; then
		inf "restoring $TARGET_USER's login shell to $_prev"
		if ! as_root chsh -s "$_prev" -- "$TARGET_USER"; then
			red "$PROG: could not restore the shell; run: sudo chsh -s $_prev $TARGET_USER"
			die "NOTHING was removed -- $DEST is still in place, so you are not locked out."
		fi
		grn "$TARGET_USER's login shell is $_prev again"
	else
		inf "$TARGET_USER's login shell is $_now, not $DEST — leaving it"
	fi
	[ -n "${HOME:-}" ] && rm -f "$HOME/.hellish-previous-shell"

	# Only now, with nothing pointing at it any more.
	if [ -e "$DEST" ]; then
		inf "removing $DEST"
		as_root rm -f "$DEST"
		[ -e "$DEST" ] && die "could not remove $DEST"
		grn "removed $DEST"
	else
		inf "no binary at $DEST"
	fi

	# chsh refuses a shell that is not listed, so the entry has to go too or
	# the next install thinks it is already registered.
	if [ -f /etc/shells ] && grep -qxF -- "$DEST" /etc/shells; then
		inf "removing $DEST from /etc/shells"
		_tmp="$(mktemp)" || die "mktemp failed"
		grep -vxF -- "$DEST" /etc/shells > "$_tmp" || true
		as_root tee /etc/shells < "$_tmp" >/dev/null
		rm -f "$_tmp"
		grn "/etc/shells no longer lists $DEST"
	fi

	if [ "$PURGE" = "1" ] && [ -n "${HOME:-}" ]; then
		inf "purging config and cached update state"
		rm -f "$HOME/.hellishrc" "$HOME/.hellish_history"
		rm -rf "$HOME/.config/hellish" "$HOME/.cache/hellish"
		grn "config removed"
	elif [ -n "${HOME:-}" ]; then
		inf "kept your ~/.hellishrc (use --purge to remove it too)"
	fi

	# The other install route puts a SECOND copy in ~/.local/bin, and it wins
	# on PATH. Uninstalling only the system one and leaving that behind is how
	# "I removed it but it is still there" happens.
	_leftover=""
	_ifs="$IFS"; IFS=:
	for _d in $PATH; do
		[ -n "$_d" ] || _d=.
		[ "$_d/hellish" = "$DEST" ] && continue
		[ -x "$_d/hellish" ] && _leftover="$_leftover $_d/hellish"
	done
	IFS="$_ifs"
	if [ -n "$_leftover" ]; then
		warn "another hellish is still on your PATH:$_leftover"
		warn "that is probably \`make user-install\` — remove it with:"
		warn "    make user-uninstall"
	fi
}

case "$ACTION" in
preflight) preflight ;;
register)  register ;;
uninstall) uninstall ;;
doctor)    doctor ;;
esac

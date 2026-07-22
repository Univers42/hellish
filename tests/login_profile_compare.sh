#!/usr/bin/env bash
# ============================================================================
# login_profile_compare.sh -- verify that a LOGIN hellish sources the same
# startup files bash does: /etc/profile (which itself runs the .sh snippets
# in /etc/profile.d) and then ~/.profile.
#
# These cases can't live in the golden category files: the harness wraps every
# line in `<shell> -c`, which is by definition a non-login shell, so it can't
# observe login-only startup at all.  Regression guarded here: hellish used to
# compute is_login_shell and then throw it away, so a chsh'd hellish silently
# lost every PATH entry the system adds at login (/snap/bin on Debian/Ubuntu)
# and everything ~/.profile exports.
#
# Machine-independent by construction: we never assert what /etc/profile
# *contains*, only that both shells end up agreeing after reading it.  HOME is
# a throwaway dir so ~/.profile is ours and no real dotfile can interfere.
#
# Runs on the host: needs only bash + hellish, no docker.  Stderr wording
# differs by shell and is not gated on.
# ============================================================================
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
HELLISH="${HELLISH:-$HERE/../build/bin/hellish}"
HELLISH="$(cd "$(dirname "$HELLISH")" 2>/dev/null && pwd)/$(basename "$HELLISH")"

if [ ! -x "$HELLISH" ]; then echo "error: hellish not found at $HELLISH" >&2; exit 2; fi

FAKE="$(mktemp -d)"
trap 'rm -rf "$FAKE"' EXIT
cat > "$FAKE/.profile" <<'EOF'
MARKER=from_dot_profile
export MARKER
PATH="$HOME/mybin:$PATH"
export PATH
EOF

# A deliberately bare PATH: whatever the two shells add on top of it comes
# from the startup files, which is exactly what we want to compare.
BASE=/usr/local/bin:/usr/bin:/bin
runner() {
	env -i HOME="$FAKE" USER="${USER:-nobody}" TERM=dumb PATH="$BASE" \
		HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 "$@"
}

pass=0; fail=0
check() {
	local name="$1" ho="$2" hx="$3" bo="$4" bx="$5"
	if [ "$ho" = "$bo" ] && [ "$hx" = "$bx" ]; then
		pass=$((pass+1)); printf '  \033[32mOK\033[0m   %s\n' "$name"
	else
		fail=$((fail+1)); printf '  \033[31mFAIL\033[0m %s\n' "$name"
		printf '        hellish: [%s] status %s\n' "$ho" "$hx"
		printf '        bash   : [%s] status %s\n' "$bo" "$bx"
	fi
}

# Each row: a label, then the argv that precedes the command string.
run_row() {
	local name="$1" cmd="$2"; shift 2
	local ho hx bo bx
	ho=$(runner "$HELLISH" "$@" -c "$cmd" 2>/dev/null); hx=$?
	bo=$(runner bash --norc "$@" -c "$cmd" 2>/dev/null); bx=$?
	check "$name" "$ho" "$hx" "$bo" "$bx"
}

printf '\n\033[1m== hellish vs bash: login-shell startup files ==\033[0m\n'

# ~/.profile is read when logging in, and NOT otherwise.
run_row "--login reads ~/.profile"      'echo "${MARKER-unset}"' --login
run_row "non-login skips ~/.profile"    'echo "${MARKER-unset}"'
# The whole point: identical PATH after startup, /snap/bin and all.
run_row "--login PATH matches bash"     'echo "$PATH"'           --login
run_row "non-login PATH untouched"      'echo "$PATH"'
# ~/.profile is sourced in *this* shell, not a subshell: its cwd/vars persist.
run_row "--login \$HOME/mybin prepended" 'case $PATH in "$HOME"/mybin:*) echo yes;; *) echo no;; esac' --login

# A missing ~/.profile must never make a login fail -- plenty of systems ship
# none, and erroring out would lock the user out of their own account.
rm -f "$FAKE/.profile"
run_row "--login without ~/.profile"    'echo alive'             --login

printf '\n'
if [ "$fail" -eq 0 ]; then
	printf '\033[1;32m  All %d login-startup cases match bash.\033[0m\n\n' "$pass"
	exit 0
fi
printf '\033[1;31m  %d/%d login-startup cases diverge from bash.\033[0m\n\n' \
	"$fail" "$((pass+fail))"
exit 1

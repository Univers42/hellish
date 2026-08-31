#!/bin/sh
# tests/installer_suite.sh -- drives install.sh end-to-end inside
# docker/Dockerfile.installer. Runs as root, which is the orchestrator's job
# only: every install under test runs as one of the two humans the image
# defines -- maxine (NOPASSWD sudoer) and nora (no sudo at all).
#
# Scenarios:
#   A  nora   --yes --plugins="git jump"   user mode picked automatically,
#                                          binary + rc hook + framework
#   B  maxine --yes --plugins=none         system mode picked automatically,
#                                          /usr/bin + /etc/shells + chsh
#   C  nora2  interactive over a pty       questions really read /dev/tty
#   D  uninstalls                          both worlds restored
#   E  one external plugin over the net    skips cleanly offline
#
# Everything hermetic: the "release" is a local server started here, serving
# the binary, its sha256, the install bundle and the plugin framework built
# into the image. Only scenario E touches the real network, and it skips.
set -u

PASS=0; FAILS=0
ok()  { PASS=$((PASS + 1)); printf 'ok   %s\n' "$1"; }
bad() { FAILS=$((FAILS + 1)); printf 'FAIL %s%s\n' "$1" "${2:+  -- $2}"; }
check() { # check <label> <command...>
	_l="$1"; shift
	if "$@" >/dev/null 2>&1; then ok "$_l"; else bad "$_l"; fi
}

# ── the fake release channel ────────────────────────────────────────────────
cd /srv/release
python3 -m http.server 8377 --bind 127.0.0.1 >/dev/null 2>&1 &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1
check "fake release server answers" \
	curl -fsS -o /dev/null http://127.0.0.1:8377/latest/download/hellish-linux-x86_64

ENV_COMMON="HELLISH_RELEASE_BASE=http://127.0.0.1:8377 \
HELLISH_PLUGINS_SRC=http://127.0.0.1:8377/hellishrc_plugins.tar.gz \
HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1"

# ── A: no sudo rights -> user mode, automatically ───────────────────────────
echo "--- A: nora (no sudo) + --yes --plugins='git jump'"
su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins='git jump'" \
	|| bad "A: install.sh exited $?"
NH=/home/nora
check "A: binary in ~/.local/bin"       test -x "$NH/.local/bin/hellish"
check "A: nothing landed in /usr/bin"   test ! -e /usr/bin/hellish
check "A: the binary answers"           su nora -c "$NH/.local/bin/hellish -c 'echo ok'"
check "A: ~/.hellishrc seeded"          test -f "$NH/.hellishrc"
check "A: rc hook block written"        grep -q "^# >>> hellish >>>" "$NH/.bashrc"
# The framework's installer replaces ~/.hellishrc with its loader and
# preserves the seeded rc as rc.d/95-previous-rc.hsh -- so the PATH block
# lives in one of the two, and both are loaded.
check "A: PATH block survives somewhere loaded" \
	sh -c "grep -rq 'local/bin' $NH/.hellishrc $NH/.hellish/rc.d/ 2>/dev/null"
check "A: framework installed"          test -d "$NH/.hellish/lib"
check "A: chosen plugin present"        test -d "$NH/.hellish/plugins/git"
check "A: unchosen builtin off in conf" grep -q "^feature net *off" "$NH/.hellish/hellish.conf"
check "A: framework loads with 0 errors" \
	su nora -c "$NH/.local/bin/hellish -c '. ~/.hellishrc >/dev/null 2>&1; [ \${#HX_ERRORS[@]} -eq 0 ]'"
check "A: conf sees git on" \
	su nora -c "$NH/.local/bin/hellish -c '. ~/.hellishrc >/dev/null 2>&1; conf list' | grep -q 'on  git'"

# ── B: passwordless sudo -> system mode, automatically ──────────────────────
echo "--- B: maxine (sudoer) + --yes --plugins=none"
PREV_SHELL="$(getent passwd maxine | cut -d: -f7)"
su maxine -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	|| bad "B: install.sh exited $?"
check "B: binary in /usr/bin"           test -x /usr/bin/hellish
check "B: /etc/shells lists it"         grep -qx /usr/bin/hellish /etc/shells
check "B: login shell changed"          sh -c '[ "$(getent passwd maxine | cut -d: -f7)" = /usr/bin/hellish ]'
check "B: the binary answers"           su maxine -c "/usr/bin/hellish -c 'echo ok'"
check "B: ~/.hellishrc seeded"          test -f /home/maxine/.hellishrc
check "B: no framework without asking"  test ! -d /home/maxine/.hellish

# ── C: the interactive path, through a real pty ─────────────────────────────
# `script` gives the child a controlling tty, so install.sh's /dev/tty reads
# are exercised for real. Two questions reach nora2: "do you have sudo
# rights?" (no) and the framework offer (no).
echo "--- C: nora2, interactive, answers n n"
printf 'n\nn\n' | su nora2 -c \
	"cd /tmp && script -qec '$ENV_COMMON sh /hellish/install.sh' /dev/null" \
	>/dev/null 2>&1 || bad "C: interactive install exited $?"
check "C: binary installed"             test -x /home/nora2/.local/bin/hellish
check "C: framework declined -> absent" test ! -d /home/nora2/.hellish

# ── D: both uninstall routes ────────────────────────────────────────────────
echo "--- D: uninstall puts both worlds back"
su nora -c "cd /tmp && sh /hellish/install.sh --uninstall" >/dev/null 2>&1 \
	|| bad "D: user uninstall exited $?"
check "D: rc hook gone"                 sh -c "! grep -q '^# >>> hellish >>>' $NH/.bashrc"
check "D: user binary gone"             test ! -e "$NH/.local/bin/hellish"
# NOT wrapped in sudo: the script escalates internally (as_root), and a
# blanket sudo would swap $HOME away from the .hellish-previous-shell
# marker it restores from.
su maxine -c "/hellish/tools/register_shell.sh --uninstall --dest /usr/bin/hellish" \
	>/dev/null 2>&1 || bad "D: system uninstall exited $?"
check "D: login shell restored"         sh -c "[ \"\$(getent passwd maxine | cut -d: -f7)\" = '$PREV_SHELL' ]"
check "D: system binary gone"           test ! -e /usr/bin/hellish

# ── E: one real external plugin, network permitting ─────────────────────────
echo "--- E: hxp install z (real network; skips cleanly offline)"
if curl -fsS -o /dev/null --max-time 10 https://raw.githubusercontent.com 2>/dev/null; then
	su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" >/dev/null 2>&1
	check "E: hxp install z fetches and wires" \
		su nora -c "$NH/.local/bin/hellish -c '. ~/.hellishrc >/dev/null 2>&1; hxp install z' && test -f $NH/.hellish/plugins/z/z.sh"
else
	echo "skip E: no network"
fi

printf '\n%d ok, %d failed\n' "$PASS" "$FAILS"
[ "$FAILS" -eq 0 ]

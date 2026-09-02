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
#   F  nora3  a clone that asks for a password  falls back to the tarball
#                                          (issue #111, on a real pty)
#   G  ines   zsh login shell + a zsh ~/.zshrc  the 42 world: hooked, imported,
#                                          and running with no parse error
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
# Not `python3 -m http.server`: this one answers git's smart-HTTP probes
# under /private/ with a 401, which is what made git ask for a username on
# a 42 machine (issue #111). See tests/helpers/fake_release_server.py.
cd /srv/release
python3 /hellish/tests/helpers/fake_release_server.py 8377 /srv/release >/dev/null 2>&1 &
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
# The framework ships its own suite (~/.hellish/test/run.hsh); a user-mode
# install -- the 42 machine -- is where it must pass, because that is the
# install with no /usr/bin/hellish for anything to hardcode.
su nora -c "$NH/.local/bin/hellish $NH/.hellish/test/run.hsh" > /tmp/A-suite.log 2>&1 \
	&& ok "A: the framework's own test suite passes on a user install" \
	|| bad "A: the framework's own test suite passes on a user install" \
		"$(grep -E 'FAIL|passed' /tmp/A-suite.log | tail -4 | tr '\n' ' ')"

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

# ── F: the clone asks for credentials -> tarball, no hang (issue #111) ───────
# git 2.34 on the 42 image takes a 401 from GitHub over HTTP/2 and then asks
# for a username on the terminal -- and `curl | sh` HAS a terminal, so the
# install sat at "Username for 'https://github.com':" until Ctrl-C. The
# fake channel answers git's probes under /private/ with exactly that 401.
# On a real pty (script), the install must neither prompt nor hang, and the
# framework must still arrive through the archive tarball beside the repo.
echo "--- F: nora3, a clone that wants a password, on a pty"
printf 'n\ny\na\n' | timeout 120 su nora3 -c \
	"cd /tmp && script -qec 'HELLISH_RELEASE_BASE=http://127.0.0.1:8377 \
HELLISH_PLUGINS_SRC=http://127.0.0.1:8377/private/hellishrc_plugins \
HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
sh /hellish/install.sh' /dev/null" > /tmp/F.log 2>&1
st=$?
check "F: the install neither hung nor failed (exit $st)" [ "$st" -eq 0 ]
check "F: no username prompt reached the terminal" \
	sh -c "! grep -q 'Username for' /tmp/F.log"
check "F: framework arrived through the tarball fallback" \
	test -d /home/nora3/.hellish/lib
check "F: chosen plugins wired"          grep -q "^feature git *on" /home/nora3/.hellish/hellish.conf

# ── G: the 42 world -- zsh login shell, a zsh-flavoured ~/.zshrc ────────────
# ines logs into zsh (as every 42 account does) and has a ~/.zshrc written
# in zsh: vcs_info, `precmd() { vcs_info }`, zstyle, PROMPT/RPROMPT -- the
# verbatim rc from issue #112 -- plus an alias of her own. Three questions
# reach her: sudo (no), load ~/.zshrc inside hellish (yes), the framework
# (no). Then an interactive hellish must run her alias with no parse error
# and no "not supported" noise: that rc is the one every prompt tutorial
# hands out, and the one a student pastes first.
echo "--- G: ines, zsh login shell, ~/.zshrc imported"
printf 'n\ny\nn\n' | timeout 120 su ines -c \
	"cd /tmp && script -qec '$ENV_COMMON sh /hellish/install.sh' /dev/null" \
	> /tmp/G.log 2>&1 || bad "G: interactive install exited $?"
IH=/home/ines
check "G: ~/.zshrc got the exec hook"     grep -q "^# >>> hellish >>>" "$IH/.zshrc"
check "G: hook parses under zsh"          su ines -c "zsh -n $IH/.zshrc"
check "G: the import module was written" test -f "$IH/.config/hellish/rc.d/90-zshrc.zsh"
printf 'hello\nexit\n' | timeout 60 su ines -c \
	"cd $IH && script -qec '$ENV_COMMON $IH/.local/bin/hellish' /dev/null" \
	> /tmp/G-shell.log 2>&1
check "G: her alias from ~/.zshrc works inside hellish" \
	grep -q "from-zshrc" /tmp/G-shell.log
check "G: the zsh rc loads with no syntax error" \
	sh -c "! grep -qi 'syntax error' /tmp/G-shell.log"
check "G: ...and with no 'not supported' noise" \
	sh -c "! grep -q 'not supported' /tmp/G-shell.log"
check "G: the hook did not re-exec inside hellish (no loop)" \
	sh -c "[ \$(grep -c 'from-zshrc' /tmp/G-shell.log) -le 2 ]"
# A declined import leaves nothing behind (nora2 said no to everything).
check "G: declining the import writes no module" \
	test ! -e /home/nora2/.config/hellish/rc.d/90-zshrc.zsh

printf '\n%d ok, %d failed\n' "$PASS" "$FAILS"
[ "$FAILS" -eq 0 ]

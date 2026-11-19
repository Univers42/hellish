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
#   H  nora4  `curl | sh` from INSIDE a stale checkout, and `sh install.sh`
#                                          of one: the release wins over an
#                                          old build/bin/hellish, always
#   I  nora4  a release whose bundle has an OLD seeder: the drivers come
#                                          from main; the bundle's only when
#                                          main is unreachable
#   J  lena   a <= 3.3.0 user-mode hellish install re-runs the installer:
#                                          updated in place under its hellish
#                                          names, nothing doubled, no polish rc
#   K  kim    a hand-written ~/.polishrc + a plugin pick: the framework is
#                                          skipped rather than shadowing it
#
# The rename (hellish -> polish, 3.4.0): a FRESH user gets the polish names
# (~/.local/bin/polish, /usr/bin/polish, ~/.polishrc, ~/.config/polish, the
# `# >>> polish >>>` hook). The plugin framework predates the rename: it
# lives in ~/.hellish/ and writes its loader to ~/.hellishrc, so those names
# stay wherever the framework is involved. An existing hellish install keeps
# its own names (J).
#
# Everything hermetic: the "release" is a local server started here, serving
# the binary under both asset names, their sha256s, the install bundle and
# the plugin framework built into the image. Only scenario E touches the
# real network, and it skips.
set -u

PASS=0; FAILS=0
ok()  { PASS=$((PASS + 1)); printf 'ok   %s\n' "$1"; }
bad() { FAILS=$((FAILS + 1)); printf 'FAIL %s%s\n' "$1" "${2:+  -- $2}"; }
check() { # check <label> <command...>
	_l="$1"; shift
	if "$@" >/dev/null 2>&1; then ok "$_l"; else bad "$_l"; fi
}
# answers USER BIN -> 0 when BIN, run as USER, prints exactly "ok".
answers() { [ "$(su "$1" -c "$2 -c 'echo ok'" 2>/dev/null)" = ok ]; }
# count_is N ERE FILE -> 0 when exactly N lines of FILE match ERE.
count_is() { [ "$(grep -cE "$2" "$3" 2>/dev/null)" -eq "$1" ]; }
# no_file PATH -> 0 when nothing, not even a dangling symlink, is there.
no_file() { [ ! -e "$1" ] && [ ! -L "$1" ]; }
# strip_path_blocks FILE -> FILE without the installer-owned PATH block
# (either marker spelling) and without trailing blank lines: the part of
# an rc that belongs to the user.
strip_path_blocks() {
	awk '/^# >>> (polish|hellish) path >>>$/ { s = 1 }
		s { if ($0 ~ /^# <<< (polish|hellish) path <<<$/) s = 0; next }
		{ print }' "$1" \
		| awk 'NF { for (; n > 0; n--) print ""; print; next } { n++ }'
}
# same_outside_path_block REF FILE -> 0 when FILE minus the installer's
# PATH block is byte-identical to REF (an already stripped copy).
same_outside_path_block() { strip_path_blocks "$2" | cmp -s "$1" -; }

# ── the fake release channel ────────────────────────────────────────────────
# Not `python3 -m http.server`: this one answers git's smart-HTTP probes
# under /private/ with a 401, which is what made git ask for a username on
# a 42 machine (issue #111). See tests/helpers/fake_release_server.py.
cd /srv/release || exit 1
python3 /hellish/tests/helpers/fake_release_server.py 8377 /srv/release >/dev/null 2>&1 &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1
check "fake release server answers" \
	curl -fsS -o /dev/null http://127.0.0.1:8377/latest/download/polish-linux-x86_64
# The channel mirrors release.yml: one binary under BOTH names (3.4.0+
# installers ask for polish-, every installed <= 3.3.0 updater for
# hellish-), each with a .sha256 that verifies it.
channel_ok() ( # channel_ok BASE_PATH -- a subshell, because it cd's
	cd "$(mktemp -d)" || exit 1
	for _a in polish-linux-x86_64 hellish-linux-x86_64; do
		curl -fsS -O "http://127.0.0.1:8377$1/latest/download/$_a" \
			&& curl -fsS -O "http://127.0.0.1:8377$1/latest/download/$_a.sha256" \
			|| exit 1
	done
	cmp polish-linux-x86_64 hellish-linux-x86_64 \
		&& sha256sum -c polish-linux-x86_64.sha256 hellish-linux-x86_64.sha256
)
check "channel: both asset names, same bytes, each checksum verifies" channel_ok ""
check "stale channel (I): both asset names, same bytes, checksums verify" channel_ok /old
REL_SUM="$(sha256sum /srv/release/latest/download/polish-linux-x86_64 | cut -d' ' -f1)"
REL_VER="$(sed -n -e 's/^# *define POLISH_VERSION "\([^"]*\)".*/\1/p' \
	-e 's/^# *define HELLISH_VERSION "\([^"]*\)".*/\1/p' /hellish/incs/version.h | head -1)"

# POLISH_RAW_BASE too: install.sh refreshes its drivers (user-install.sh
# and friends) from raw.githubusercontent on top of the bundle, so with the
# network up every scenario was quietly running MAIN's scripts, not this
# tree's -- a fix in user-install.sh could not pass here until it was
# already pushed. The fake channel serves the tree's copies under /raw.
# F and I spell the install knobs the pre-3.4 way (HELLISH_*) on purpose:
# install.sh reads POLISH_<X>, else HELLISH_<X>, and both must keep working.
ENV_COMMON="POLISH_RELEASE_BASE=http://127.0.0.1:8377 \
POLISH_RAW_BASE=http://127.0.0.1:8377/raw \
POLISH_PLUGINS_SRC=http://127.0.0.1:8377/hellishrc_plugins.tar.gz \
POLISH_NO_BANNER=1 POLISH_NO_UPDATE_CHECK=1 POLISH_NO_ANIM=1 \
HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1"

# ── A: no sudo rights -> user mode, automatically ───────────────────────────
echo "--- A: nora (no sudo) + --yes --plugins='git jump'"
su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins='git jump'" \
	> /tmp/A.log 2>&1 || bad "A: install.sh exited $?"
cat /tmp/A.log
NH=/home/nora
check "A: binary in ~/.local/bin, as polish" test -x "$NH/.local/bin/polish"
check "A: ...and no hellish name on a fresh install" no_file "$NH/.local/bin/hellish"
check "A: nothing landed in /usr/bin" \
	sh -c "[ ! -e /usr/bin/polish ] && [ ! -e /usr/bin/hellish ]"
check "A: the binary answers"           answers nora "$NH/.local/bin/polish"
check "A: rc hook block written"        grep -q "^# >>> polish >>>" "$NH/.bashrc"
check "A: ...and no legacy hellish hook" sh -c "! grep -q '^# >>> hellish >>>' $NH/.bashrc"
# The seeder made ~/.polishrc; the framework installs its loader into
# ~/.hellishrc, which polish only reads while no ~/.polishrc exists. So the
# installer moves the rc it seeded THIS run to ~/.hellishrc first, and the
# framework keeps that content as rc.d/95-previous-rc.hsh.
check "A: the seeded ~/.polishrc moved to ~/.hellishrc for the framework" \
	grep -q "moved ~/.polishrc to ~/.hellishrc" /tmp/A.log
check "A: ...so no ~/.polishrc shadows the framework's loader" no_file "$NH/.polishrc"
check "A: ~/.hellishrc is there (the framework's loader)" test -f "$NH/.hellishrc"
# The framework's installer replaces ~/.hellishrc with its loader and
# preserves the seeded rc as rc.d/95-previous-rc.hsh -- so the PATH block
# lives in one of the two, and both are loaded.
check "A: PATH block survives somewhere loaded" \
	sh -c "grep -rq 'local/bin' $NH/.hellishrc $NH/.hellish/rc.d/ 2>/dev/null"
check "A: framework installed"          test -d "$NH/.hellish/lib"
check "A: chosen plugin present"        test -d "$NH/.hellish/plugins/git"
check "A: unchosen builtin off in conf" grep -q "^feature net *off" "$NH/.hellish/hellish.conf"
check "A: framework loads with 0 errors" \
	su nora -c "$NH/.local/bin/polish -c '. ~/.hellishrc >/dev/null 2>&1; [ \${#HX_ERRORS[@]} -eq 0 ]'"
check "A: conf sees git on" \
	su nora -c "$NH/.local/bin/polish -c '. ~/.hellishrc >/dev/null 2>&1; conf list' | grep -q 'on  git'"
# A RE-RUN over the framework's own rc. ~/.hellishrc is now the loader,
# whose `HX_LOADED=()` is bash syntax; the seeder used to validate the rc
# with `sh -n` (dash), refuse its PATH block, and -- under the caller's
# set -e -- stop the whole install right there, binary copied and nothing
# hooked or asked. The second install must run to the end.
if su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	> /tmp/A-rerun.log 2>&1; then
	ok "A: a re-run over the framework rc completes"
else
	bad "A: a re-run over the framework rc completes" "exit $? -- $(grep -E 'would not parse|error' /tmp/A-rerun.log | head -1)"
fi
# ~/.hellishrc is the rc polish reads here, and the PATH block's markers
# follow the rc's own name -- so it is the hellish-spelled block.
check "A: ...and the PATH block is written into it" \
	sh -c "grep -q 'hellish path >>>' $NH/.hellishrc"
check "A: ...exactly one PATH block in it" \
	count_is 1 '^# >>> (polish|hellish) path >>>$' "$NH/.hellishrc"
check "A: ...the rc hook is still in place, once" \
	count_is 1 '^# >>> (polish|hellish) >>>$' "$NH/.bashrc"
check "A: ...a re-run did not recreate ~/.polishrc" no_file "$NH/.polishrc"
check "A: ...and the framework is still there" test -d "$NH/.hellish/lib"
# The framework ships its own suite (~/.hellish/test/run.hsh); a user-mode
# install -- the 42 machine -- is where it must pass, because that is the
# install with no /usr/bin binary for anything to hardcode.
if su nora -c "$NH/.local/bin/polish $NH/.hellish/test/run.hsh" > /tmp/A-suite.log 2>&1; then
	ok "A: the framework's own test suite passes on a user install"
else
	bad "A: the framework's own test suite passes on a user install" \
		"$(grep -E 'FAIL|passed' /tmp/A-suite.log | tail -4 | tr '\n' ' ')"
fi

# ── B: passwordless sudo -> system mode, automatically ──────────────────────
echo "--- B: maxine (sudoer) + --yes --plugins=none"
PREV_SHELL="$(getent passwd maxine | cut -d: -f7)"
su maxine -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	|| bad "B: install.sh exited $?"
check "B: binary in /usr/bin, as polish" test -x /usr/bin/polish
check "B: ...and no hellish name on a fresh machine" no_file /usr/bin/hellish
check "B: /etc/shells lists it"         grep -qx /usr/bin/polish /etc/shells
check "B: login shell changed"          test "$(getent passwd maxine | cut -d: -f7)" = /usr/bin/polish
check "B: the binary answers"           answers maxine /usr/bin/polish
check "B: ~/.polishrc seeded"           test -f /home/maxine/.polishrc
check "B: ...and no ~/.hellishrc beside it" no_file /home/maxine/.hellishrc
check "B: no framework without asking"  test ! -d /home/maxine/.hellish

# ── C: the interactive path, through a real pty ─────────────────────────────
# `script` gives the child a controlling tty, so install.sh's /dev/tty reads
# are exercised for real. Two questions reach nora2: "do you have sudo
# rights?" (no) and the framework offer (no).
echo "--- C: nora2, interactive, answers n n"
printf 'n\nn\n' | su nora2 -c \
	"cd /tmp && script -qec '$ENV_COMMON sh /hellish/install.sh' /dev/null" \
	>/dev/null 2>&1 || bad "C: interactive install exited $?"
check "C: binary installed"             test -x /home/nora2/.local/bin/polish
check "C: framework declined -> absent" test ! -d /home/nora2/.hellish

# ── D: both uninstall routes ────────────────────────────────────────────────
echo "--- D: uninstall puts both worlds back"
# A 2.7.x install left its own hook generation behind (issue #116): both
# older marker pairs, in both rc files, must go with the uninstall too.
su nora -c "printf '# >>> hellish-default >>>\nexec /old/hellish\n# <<< hellish-default <<<\n' >> $NH/.bashrc; printf '# >>> hellish-user-default >>>\nexec /old/hellish\n# <<< hellish-user-default <<<\n' >> $NH/.zshrc"
su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --uninstall" >/dev/null 2>&1 \
	|| bad "D: user uninstall exited $?"
check "D: rc hook gone"                 sh -c "! grep -qE '^# >>> (polish|hellish) >>>' $NH/.bashrc"
check "D: 2.7.x hooks gone from both rc files" \
	sh -c "! grep -q 'hellish-default\|hellish-user-default' $NH/.bashrc $NH/.zshrc"
check "D: ...with backups beside them" test -f "$NH/.bashrc.hellish-legacy-bak"
check "D: user binary gone, under both names" \
	sh -c "[ ! -e $NH/.local/bin/polish ] && [ ! -L $NH/.local/bin/polish ] && [ ! -e $NH/.local/bin/hellish ]"
# NOT wrapped in sudo: the script escalates internally (as_root), and a
# blanket sudo would swap $HOME away from the .polish-previous-shell
# marker it restores from.
su maxine -c "/hellish/tools/register_shell.sh --uninstall --dest /usr/bin/polish" \
	>/dev/null 2>&1 || bad "D: system uninstall exited $?"
check "D: login shell restored"         sh -c "[ \"\$(getent passwd maxine | cut -d: -f7)\" = '$PREV_SHELL' ]"
check "D: system binary gone, under both names" \
	sh -c "[ ! -e /usr/bin/polish ] && [ ! -L /usr/bin/polish ] && [ ! -e /usr/bin/hellish ]"
check "D: /etc/shells no longer lists it" sh -c "! grep -qx /usr/bin/polish /etc/shells"

# ── E: one real external plugin, network permitting ─────────────────────────
echo "--- E: hxp install z (real network; skips cleanly offline)"
if curl -fsS -o /dev/null --max-time 10 https://raw.githubusercontent.com 2>/dev/null; then
	su nora -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" >/dev/null 2>&1
	check "E: hxp install z fetches and wires" \
		su nora -c "$NH/.local/bin/polish -c '. ~/.hellishrc >/dev/null 2>&1; hxp install z' && test -f $NH/.hellish/plugins/z/z.sh"
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
# verbatim rc from issue #112 -- plus an alias of her own. Two questions
# reach her: sudo (no) and the framework (no). ~/.zshrc is zsh's file and
# the default install does not touch it: no import is written, and her
# shell starts with polish's own config (issues #114-#116 are what the
# old default did). The import is `--zshrc`, opt-in and best effort: with
# it, an interactive polish must run her alias with no parse error and no
# "not supported" noise. A re-run WITHOUT the flag then switches the
# import off, keeping the file as .off, and says so. Hers is a fresh
# install, so the config dir is ~/.config/polish.
echo "--- G: ines, zsh login shell: no import by default, --zshrc opts in"
printf 'n\nn\n' | timeout 120 su ines -c \
	"cd /tmp && script -qec '$ENV_COMMON sh /hellish/install.sh' /dev/null" \
	> /tmp/G.log 2>&1 || bad "G: interactive install exited $?"
IH=/home/ines
IX="$IH/.config/polish"
check "G: ~/.zshrc got the exec hook"     grep -q "^# >>> polish >>>" "$IH/.zshrc"
check "G: hook parses under zsh"          su ines -c "zsh -n $IH/.zshrc"
check "G: the default install writes NO ~/.zshrc import" \
	sh -c "test ! -e $IX/after.d/90-zshrc.zsh && test ! -e $IX/rc.d/90-zshrc.zsh"
check "G: ...and says the zsh config stays zsh's" \
	grep -q "stays zsh's" /tmp/G.log
printf 'alias hello\necho started\nexit\n' | timeout 60 su ines -c \
	"cd $IH && script -qec '$ENV_COMMON $IH/.local/bin/polish' /dev/null" \
	> /tmp/G-shell0.log 2>&1
check "G: without the import, her zsh alias is NOT in polish" \
	sh -c "! grep -q 'from-zshrc' /tmp/G-shell0.log"
check "G: ...and the shell started clean" \
	sh -c "grep -q 'started' /tmp/G-shell0.log && ! grep -qiE 'syntax error|not supported' /tmp/G-shell0.log"
su ines -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none --zshrc" \
	> /tmp/G2.log 2>&1 || bad "G: --zshrc install exited $?"
check "G: --zshrc writes the import, in after.d" \
	test -f "$IX/after.d/90-zshrc.zsh"
printf 'hello\nexit\n' | timeout 60 su ines -c \
	"cd $IH && script -qec '$ENV_COMMON $IH/.local/bin/polish' /dev/null" \
	> /tmp/G-shell.log 2>&1
check "G: with --zshrc, her alias from ~/.zshrc works inside polish" \
	grep -q "from-zshrc" /tmp/G-shell.log
check "G: the zsh rc loads with no syntax error" \
	sh -c "! grep -qi 'syntax error' /tmp/G-shell.log"
check "G: ...and with no 'not supported' noise" \
	sh -c "! grep -q 'not supported' /tmp/G-shell.log"
check "G: the hook did not re-exec inside polish (no loop)" \
	sh -c "[ \$(grep -c 'from-zshrc' /tmp/G-shell.log) -le 2 ]"
# An earlier import, then a plain re-run: switched off, kept as .off, told.
su ines -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	> /tmp/G3.log 2>&1 || bad "G: re-run install exited $?"
check "G: a plain re-run switches the import off" \
	sh -c "test ! -e $IX/after.d/90-zshrc.zsh && test -e $IX/after.d/90-zshrc.zsh.off"
check "G: ...and says so" grep -q "no longer loaded" /tmp/G3.log
# A legacy rc.d import from 2.8.x is switched off the same way.
su ines -c "mkdir -p $IX/rc.d && printf 'source ~/.zshrc\n' > $IX/rc.d/90-zshrc.zsh"
su ines -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" >/dev/null 2>&1
check "G: a 2.8.x rc.d import is switched off too" \
	sh -c "test ! -e $IX/rc.d/90-zshrc.zsh && test -e $IX/rc.d/90-zshrc.zsh.off"
# nora2 said no to everything: nothing written either.
check "G: declining everything writes no module" \
	test ! -e /home/nora2/.config/polish/after.d/90-zshrc.zsh

# ── H: an old checkout must never win over the release ──────────────────────
# `update --now` runs the one-liner in the shell's cwd. Under `curl | sh`
# there is no script file, so $0 is "sh" and dirname("sh") is "." -- and the
# installer took the cwd for a source checkout, installing ITS
# build/bin/hellish. A developer who typed `update --now` inside an August
# checkout got 2.3.2 back on a machine whose release channel said 2.8.7.
# Two shapes, both against a fake pre-3.4 checkout whose build claims to be
# 2.3.2: piped from inside it (must download, not even look at the build),
# and `sh install.sh` of it (a build that disagrees with incs/version.h is
# refused and the release fetched instead).
echo "--- H: nora4, an update from inside a stale checkout"
SC=/home/nora4/old-checkout
mkdir -p "$SC/build/bin" "$SC/incs" "$SC/tools"
cp /hellish/install.sh /hellish/user-install.sh /hellish/hellishrc.example "$SC/"
cp /hellish/tools/register_shell.sh /hellish/tools/seed_hellishrc.sh "$SC/tools/"
cp -R /hellish/share "$SC/share"
: > "$SC/Makefile"
cat > "$SC/build/bin/hellish" <<'EOF'
#!/bin/sh
case "$1" in --version) echo "hellish, version 2.3.2 (stale)";; -c) shift; eval "$1";; esac
EOF
chmod +x "$SC/build/bin/hellish" "$SC/tools/"*.sh "$SC/"*.sh
printf '# define HELLISH_VERSION "9.9.9"\n' > "$SC/incs/version.h"
chown -R nora4:nora4 "$SC"
NH4=/home/nora4
# H1: piped, cwd = the stale checkout
su nora4 -c "cd $SC && cat /hellish/install.sh | $ENV_COMMON sh -s -- --yes --plugins=none" \
	> /tmp/H1.log 2>&1 || bad "H1: piped install exited $?"
check "H1: piped from inside a checkout still downloads" grep -q "downloading" /tmp/H1.log
check "H1: ...and installs the RELEASE binary, not the checkout's" \
	sh -c "[ \"\$(sha256sum $NH4/.local/bin/polish | cut -d' ' -f1)\" = '$REL_SUM' ]"
su nora4 -c "cd /tmp && sh /hellish/install.sh --uninstall" >/dev/null 2>&1
# H2: sh install.sh of the checkout, whose build disagrees with version.h
su nora4 -c "cd /tmp && $ENV_COMMON sh $SC/install.sh --yes --plugins=none" \
	> /tmp/H2.log 2>&1 || bad "H2: checkout install exited $?"
check "H2: a stale checkout build is refused, by name" grep -q "not installing it" /tmp/H2.log
check "H2: ...and the release is installed instead" \
	sh -c "[ \"\$(sha256sum $NH4/.local/bin/polish | cut -d' ' -f1)\" = '$REL_SUM' ]"
check "H2: the installed shell is the release version" \
	su nora4 -c "$NH4/.local/bin/polish --version | grep -qv stale"
# H3: a forgotten old hellish EARLIER on PATH than the install directory
# is named, loudly -- it is what keeps answering `hellish` afterwards.
mkdir -p "$NH4/bin" && cp "$SC/build/bin/hellish" "$NH4/bin/hellish" && chown -R nora4:nora4 "$NH4/bin"
su nora4 -c "cd /tmp && PATH=$NH4/bin:\$PATH $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	> /tmp/H3.log 2>&1 || bad "H3: install exited $?"
check "H3: an older hellish ahead on PATH is called out" \
	grep -q "AHEAD of $NH4/.local/bin/polish on your PATH: $NH4/bin/hellish (2.3.2)" /tmp/H3.log
rm -f "$NH4/bin/hellish"

# ── I: the drivers come from main, not from the release's bundle ─────────────
# install.sh is fetched from main; the bundle's scripts are frozen at the
# release. A seeder fix on main was therefore unreachable: every
# `curl | sh` still ran the latest release's broken copy, and the user who
# reported it saw the same failure again after the fix was pushed. The
# stale channel here ships a bundle whose seeder announces itself; with
# raw reachable it must NOT run, without raw it is the fallback that does.
echo "--- I: nora4, a release whose bundle carries an old seeder"
su nora4 -c "cd /tmp && sh /hellish/install.sh --uninstall" >/dev/null 2>&1
su nora4 -c "cd /tmp && HELLISH_RELEASE_BASE=http://127.0.0.1:8377/old \
HELLISH_RAW_BASE=http://127.0.0.1:8377/raw \
HELLISH_PLUGINS_SRC=http://127.0.0.1:8377/hellishrc_plugins.tar.gz \
HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
sh /hellish/install.sh --yes --plugins=none" > /tmp/I1.log 2>&1 || bad "I1: install exited $?"
check "I1: the bundle's stale seeder did not run"  sh -c "! grep -q OLD-BUNDLE-SEEDER /tmp/I1.log"
check "I1: ...main's drivers did (the block was written)" grep -q "puts /home/nora4/.local/bin on PATH" /tmp/I1.log
check "I1: no warning about unreachable scripts" sh -c "! grep -q 'could not refresh' /tmp/I1.log"
su nora4 -c "cd /tmp && sh /hellish/install.sh --uninstall" >/dev/null 2>&1
su nora4 -c "cd /tmp && HELLISH_RELEASE_BASE=http://127.0.0.1:8377/old \
HELLISH_RAW_BASE=http://127.0.0.1:1/raw \
HELLISH_PLUGINS_SRC=http://127.0.0.1:8377/hellishrc_plugins.tar.gz \
HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
sh /hellish/install.sh --yes --plugins=none" > /tmp/I2.log 2>&1 || bad "I2: install exited $?"
check "I2: with main unreachable, the bundle's seeder is the fallback" grep -q OLD-BUNDLE-SEEDER /tmp/I2.log
check "I2: ...and the installer said so" grep -q "could not refresh" /tmp/I2.log
check "I2: ...and still installed the shell" test -x /home/nora4/.local/bin/polish

# ── J: a <= 3.3.0 user-mode install upgrades in place ───────────────────────
# lena is set up exactly as 3.3.0's user-install.sh left a no-sudo user:
# ~/.local/bin/hellish, ~/.hellishrc (her own line plus the hellish PATH
# block), ~/.config/hellish/, and the `# >>> hellish >>>` exec hook at the
# end of ~/.bashrc -- block text as 3.3.0 wrote it, comments dropped. The
# binary is a stand-in that answers like 3.3.0 and runs commands; what
# matters is that it is NOT the release, so the sha256 proves the update.
# Her hook, her PATH and her old updater all name ~/.local/bin/hellish, so
# the rename must update THAT file, add nothing beside her rc, and double
# neither block.
echo "--- J: lena, a <= 3.3.0 user-mode hellish install, re-runs the installer"
LH=/home/lena
mkdir -p "$LH/.local/bin" "$LH/.config/hellish/rc.d"
cat > "$LH/.local/bin/hellish" <<'EOF'
#!/bin/sh
case "$1" in --version) echo "hellish, version 3.3.0";; -c) shift; exec /bin/sh -c "$@";; *) exec /bin/sh "$@";; esac
EOF
chmod 755 "$LH/.local/bin/hellish"
cat > "$LH/.hellishrc" <<'EOF'
# lena's own config, written long before the rename
alias ll='ls -l'  # J-USER-MARKER

# >>> hellish path >>>
case ":$PATH:" in
	*":/home/lena/.local/bin:"*) ;;
	*) PATH="/home/lena/.local/bin:$PATH"; export PATH ;;
esac
# <<< hellish path <<<
EOF
printf "PS1='lena> '\n" > "$LH/.config/hellish/rc.d/30-prompt.hsh"
cat >> "$LH/.bashrc" <<'EOF'

# >>> hellish >>>
case ":$PATH:" in
	*":/home/lena/.local/bin:"*) ;;
	*) PATH="/home/lena/.local/bin:$PATH"; export PATH ;;
esac

if [ -z "${HELLISH_NO_EXEC-}" ] && [ -z "${HELLISH_EXECD-}" ] \
	&& [ ! -e "$HOME/.hellish-disable" ] && [ -x "/home/lena/.local/bin/hellish" ]; then
	case $- in
	*i*)
		HELLISH_EXECD=1
		export HELLISH_EXECD
		SHELL="/home/lena/.local/bin/hellish"
		export SHELL
		exec "/home/lena/.local/bin/hellish"
		;;
	esac
fi
# <<< hellish <<<
EOF
chown -R lena:lena "$LH"
strip_path_blocks "$LH/.hellishrc" > /tmp/J.hellishrc.user
OLD_SUM="$(sha256sum "$LH/.local/bin/hellish" | cut -d' ' -f1)"
su lena -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins=none" \
	> /tmp/J.log 2>&1 || bad "J: install.sh exited $?" "$(tail -3 /tmp/J.log | tr '\n' ' ')"
cat /tmp/J.log
check "J: the stand-in was not the release to begin with" [ "$OLD_SUM" != "$REL_SUM" ]
check "J: ~/.local/bin/hellish is now the release (sha256)" \
	sh -c "[ ! -L $LH/.local/bin/hellish ] && [ \"\$(sha256sum $LH/.local/bin/hellish | cut -d' ' -f1)\" = '$REL_SUM' ]"
check "J: ~/.local/bin/polish is only an alias of it (symlink, one binary)" \
	sh -c "[ -L $LH/.local/bin/polish ] && [ \"\$(readlink -f $LH/.local/bin/polish)\" = $LH/.local/bin/hellish ]"
check "J: ~/.hellishrc still has her own line" \
	grep -qxF "alias ll='ls -l'  # J-USER-MARKER" "$LH/.hellishrc"
check "J: ...and is unchanged outside the installer's PATH block" \
	same_outside_path_block /tmp/J.hellishrc.user "$LH/.hellishrc"
check "J: ...with exactly one PATH block, still the hellish-named one" \
	sh -c "grep -qx '# >>> hellish path >>>' $LH/.hellishrc && [ \$(grep -cE '^# >>> (polish|hellish) path >>>\$' $LH/.hellishrc) -eq 1 ]"
check "J: NO ~/.polishrc was created" no_file "$LH/.polishrc"
check "J: NO ~/.config/polish was created" no_file "$LH/.config/polish"
check "J: ~/.bashrc has exactly one rc hook, still the hellish-named one" \
	sh -c "grep -qx '# >>> hellish >>>' $LH/.bashrc && [ \$(grep -cE '^# >>> (polish|hellish) >>>\$' $LH/.bashrc) -eq 1 ]"
check "J: ...and exactly one PATH prepend in ~/.bashrc" \
	count_is 1 "PATH=\"/home/lena/.local/bin:\\\$PATH\"" "$LH/.bashrc"
check "J: ...which still execs ~/.local/bin/hellish" \
	grep -qF 'exec "/home/lena/.local/bin/hellish"' "$LH/.bashrc"
check "J: the shell still starts (hellish -c 'echo ok')" \
	answers lena "$LH/.local/bin/hellish"
# The old-updater contract. Every installed <= 3.3.0 updater validates a
# download by running `<bin> -c 'update --version'` and comparing the word
# after the first space with the version it asked for (update_apply.c,
# validate_binary). Two words, no "v", one line.
J_UPV="$(su lena -c "$LH/.local/bin/hellish -c 'update --version'" 2>/dev/null)"
if [ -n "$REL_VER" ] && [ "$J_UPV" = "polish $REL_VER" ]; then
	ok "J: '<installed> update --version' prints exactly 'polish $REL_VER'"
else
	bad "J: '<installed> update --version' prints exactly 'polish $REL_VER'" "got '$J_UPV'"
fi

# ── K: a hand-written ~/.polishrc is never shadowed by the framework ────────
# The framework writes its loader to ~/.hellishrc, which polish ignores
# while a ~/.polishrc exists. kim wrote her ~/.polishrc herself before any
# installer ran, and asks for a plugin: the installer must neither move her
# rc out of the way nor install a framework that would never load -- it
# skips the framework and says so. The seeder's one licence still holds:
# it may (re)write the PATH block between its own markers, so the rc is
# compared byte for byte OUTSIDE that block (and the raw sha256 reported).
echo "--- K: kim, a hand-written ~/.polishrc, asks for the git plugin"
KH=/home/kim
printf '# kim wrote this before any installer ran\nalias k="echo kim"  # K-USER-MARKER\n' \
	> "$KH/.polishrc"
chown kim:kim "$KH/.polishrc"
cp "$KH/.polishrc" /tmp/K.polishrc.orig
K_SUM="$(sha256sum < "$KH/.polishrc")"
su kim -c "cd /tmp && $ENV_COMMON sh /hellish/install.sh --yes --plugins='git'" \
	> /tmp/K.log 2>&1 || bad "K: install.sh exited $?" "$(tail -3 /tmp/K.log | tr '\n' ' ')"
cat /tmp/K.log
check "K: ~/.polishrc still exists, with her marker" \
	grep -qF 'K-USER-MARKER' "$KH/.polishrc"
check "K: ...byte-identical outside the installer's own PATH block" \
	same_outside_path_block /tmp/K.polishrc.orig "$KH/.polishrc"
if [ "$(sha256sum < "$KH/.polishrc")" = "$K_SUM" ]; then
	echo "info K: ~/.polishrc sha256 unchanged"
else
	echo "info K: ~/.polishrc sha256 changed -- the diff is the seeder's PATH block:"
	diff /tmp/K.polishrc.orig "$KH/.polishrc" | grep -E '^[<>] # (>>>|<<<)' || true
fi
check "K: NO ~/.hellishrc was created" no_file "$KH/.hellishrc"
check "K: the log says the framework was skipped" \
	grep -q "plugin framework skipped" /tmp/K.log
check "K: ...and it really was (no ~/.hellish/lib)" test ! -d "$KH/.hellish/lib"
check "K: binary installed, as polish" test -x "$KH/.local/bin/polish"
check "K: ...and it answers" answers kim "$KH/.local/bin/polish"

printf '\n%d ok, %d failed\n' "$PASS" "$FAILS"
[ "$FAILS" -eq 0 ]

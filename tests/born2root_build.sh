#!/usr/bin/env bash
# ============================================================================
# tests/born2root_build.sh -- build born2root's VM, driven by hellish, for real.
#
# tests/born2root_check.sh proves the scripts parse and the self-contained
# ones print what bash prints. This is the rest: `make all` launched FROM
# hellish, so born2root's shell probe picks hellish and every host-side
# script -- the dependency check, the ISO builder, the QEMU driver or the
# VirtualBox orchestrator, the install tracker, the first boot, ssh-config,
# the Inception host access -- runs under it, end to end, until a Debian
# guest answers over ssh with hellish as its login shell. Then the guest is
# asked the questions the ssh-as-login-shell contract needs (scp, sftp, an
# interactive login through a pty), the parity table is printed, and the
# guest is shut down.
#
# Two backends, two script families: `qemu` (setup/host/qemu_vm.sh,
# qemu_pipeline.sh) and `virtualbox` (generate/orchestrate.sh,
# setup/install/vms/install_vm_debian.sh, unlock_vm.sh). BACKEND picks one,
# or `both` runs them one after the other. VirtualBox is confined to the
# throwaway HOME (VBOX_USER_HOME + its own VBoxSVC), so nothing is registered
# in the real user's VirtualBox.
#
# Then Inception, the project that VM exists to run: `make inception` from
# hellish uploads tests/inception into the guest and builds the docker stack
# there, with Inception's Makefile running its recipes under hellish.real
# (the guest's real login shell). Inception's own compliance suite is then
# run against the live stack twice inside the guest -- under hellish.real
# and under /bin/sh -- and the two reports are diffed; born2root's
# verify_access closes the loop from the host.
#
# Before the VM, the one host-side step whose output is a file -- the
# preseeded ISO -- is built under bash too, and the two ISO trees are diffed
# byte for byte (initrd.gz compared decompressed with cpio's per-run header
# fields masked).
#
# It needs what a person needs: /dev/kvm or /dev/vboxdrv, xorriso, the
# network for the Debian packages the installer fetches, and ten to thirty
# minutes per backend. It is not in ci.yml; run it on a machine that can
# build the VM (the born2root-vm workflow does, weekly). Exit 0 with a notice
# when the submodule is absent, 2 when the host cannot do it, 1 on a real
# failure.
#
#   tests/born2root_build.sh                 qemu: build, verify, stop the guest
#   BORN2ROOT_BACKEND=virtualbox tests/born2root_build.sh
#   BORN2ROOT_BACKEND=both tests/born2root_build.sh --purge
#
# Knobs:
#   BORN2ROOT_BACKEND      qemu (default) | virtualbox | both
#   HELLISH_BIN            the hellish that launches make   (build/bin/hellish)
#   GUEST_SHELL            binary to bake as the guest's login shell; default
#                          dist/hellish-linux-<amd64|arm64> when `make static`
#                          made one, else born2root downloads its release
#   BORN2ROOT_ISO_CACHE    dir holding debian-*-amd64-netinst.iso, hardlinked
#                          in so nothing is downloaded twice (~/.cache/born2root)
#   BORN2ROOT_WORK         fake HOME, VM disks, ISO trees   (build/born2root;
#                          `make re` wipes it, so point it elsewhere to keep
#                          a guest across rebuilds)
#   BORN2ROOT_SKIP_ISO_PARITY=1   skip the bash ISO build and the tree diff
#   BORN2ROOT_REUSE_VM=1   a guest already built under BORN2ROOT_WORK is
#                          booted (from hellish) instead of rebuilt: the
#                          guest checks alone, in a few minutes
#   BORN2ROOT_INCEPTION=0  skip the Inception deployment (on by default when
#                          tests/inception is checked out)
#   BORN2ROOT_TRIPWIRE_LOG a file /bin/bash appends to for every invocation
#                          made under BORN2ROOT_TRIPWIRE (docker/Dockerfile
#                          .born2root installs such a bash); when set, a
#                          non-empty log after a phase is a failure
#   --purge                delete the work dir (and unregister the VirtualBox
#                          VM) after a clean run
# ============================================================================
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
H="${HELLISH_BIN:-$ROOT/build/bin/hellish}"
B2R="$ROOT/tests/born2root"
WORK="${BORN2ROOT_WORK:-$ROOT/build/born2root}"
CACHE="${BORN2ROOT_ISO_CACHE:-$HOME/.cache/born2root}"
BACKENDS="${BORN2ROOT_BACKEND:-qemu}"
[ "$BACKENDS" = both ] && BACKENDS="qemu virtualbox"
VM_NAME=debian
PURGE=0
[ "${1:-}" = "--purge" ] && PURGE=1

say() { printf '\n== %s  (%s)\n' "$*" "$(date +%H:%M:%S)"; }
ok() { printf 'ok    %s\n' "$*"; }
ko() { printf 'FAIL  %s\n' "$*"; fail=1; }
fail=0

# ---- can this host do it? --------------------------------------------------
if [ ! -f "$B2R/Makefile" ]; then
	echo "born2root: submodule not checked out (git submodule update --init tests/born2root) -- skipping"
	exit 0
fi
[ -x "$H" ] || { echo "error: hellish not built at $H (make all)" >&2; exit 2; }
for t in xorriso curl ssh ssh-keygen python3 make; do
	command -v "$t" >/dev/null 2>&1 || { echo "error: $t not installed" >&2; exit 2; }
done
for b in $BACKENDS; do
	case "$b" in
	qemu)
		for t in qemu-system-x86_64 qemu-img; do
			command -v "$t" >/dev/null 2>&1 || { echo "error: $t not installed" >&2; exit 2; }
		done
		[ -c /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ] \
			|| { echo "error: /dev/kvm is not usable by $(id -un)" >&2; exit 2; } ;;
	virtualbox)
		# /dev/vboxdrv is root-only by design (the setuid VM processes open
		# it; users get /dev/vboxdrvu), so "is the module loaded" is the
		# question, and VBoxManage answers it with a WARNING when not.
		# `make all` runs born2root's own check_driver after this.
		command -v VBoxManage >/dev/null 2>&1 || { echo "error: VBoxManage not installed" >&2; exit 2; }
		mkdir -p "$WORK/home/.config/VirtualBox"
		vv="$(VBOX_USER_HOME="$WORK/home/.config/VirtualBox" VBOX_IPC_SOCKETID=born2root-build VBoxManage --version 2>&1)"
		[ -c /dev/vboxdrv ] && ! printf '%s' "$vv" | grep -q 'kernel module' \
			|| { echo "error: VirtualBox's kernel driver is not usable: ${vv:-no VBoxManage output}" >&2; exit 2; } ;;
	*) echo "error: BORN2ROOT_BACKEND must be qemu, virtualbox or both" >&2; exit 2 ;;
	esac
done
H="$(cd "$(dirname "$H")" && pwd)/$(basename "$H")"

# ---- a home of its own -----------------------------------------------------
# born2root writes ~/.ssh/config, and the last step of `make all` configures
# browsers, a user systemd unit and ~/.local/bin. None of that belongs in the
# home of whoever runs a test, so the run gets a fresh HOME with its own key
# (baked into the ISO, so `ssh b2b` works) and no session bus to reach.
# VirtualBox keeps its registry there too, under its own VBoxSVC.
FAKE="$WORK/home"
mkdir -p "$FAKE/.ssh" "$FAKE/.config/VirtualBox" "$WORK/dist" "$WORK/bin"
chmod 700 "$FAKE/.ssh"
[ -f "$FAKE/.ssh/id_ed25519" ] \
	|| ssh-keygen -q -t ed25519 -N '' -C born2root-build -f "$FAKE/.ssh/id_ed25519"
touch "$FAKE/.ssh/config"; chmod 600 "$FAKE/.ssh/config"
# OpenSSH finds ~/.ssh through the passwd entry, not $HOME, so the config
# born2root writes into the fake home would never be read and the key baked
# into the ISO never offered. These wrappers, first on PATH, point every
# `ssh b2b` the scripts (and this file) run at the fake home.
for t in ssh scp sftp; do
	printf '#!/bin/sh\nexec /usr/bin/%s -F "%s/.ssh/config" -i "%s/.ssh/id_ed25519" "$@"\n' \
		"$t" "$FAKE" "$FAKE" >"$WORK/bin/$t"
	chmod 755 "$WORK/bin/$t"
done

# ---- the ISO cache, and the guest shell -----------------------------------
if [ -d "$CACHE" ]; then
	for iso in "$CACHE"/debian-*-amd64-netinst.iso; do
		[ -f "$iso" ] || continue
		[ -f "$B2R/$(basename "$iso")" ] \
			|| ln "$iso" "$B2R/$(basename "$iso")" 2>/dev/null \
			|| cp "$iso" "$B2R/$(basename "$iso")"
	done
fi
# `make static` names its output the way the release does: amd64 / arm64.
arch="$(uname -m)"
case "$arch" in x86_64) arch=amd64 ;; aarch64) arch=arm64 ;; esac
GUEST="${GUEST_SHELL:-$ROOT/dist/hellish-linux-$arch}"
SHELL_ARGS=()
if [ -f "$GUEST" ]; then
	# fetch_hellish.sh would replace CUSTOM_SHELL_PATH with the published
	# release unless the version stamp beside it matches HELLISH_VERSION.
	cp "$GUEST" "$WORK/dist/hellish" && chmod 755 "$WORK/dist/hellish"
	ver="local-$(git -C "$ROOT" describe --tags --always --dirty 2>/dev/null || echo tree)"
	printf '%s\n' "$ver" >"$WORK/dist/.hellish-version"
	SHELL_ARGS=("CUSTOM_SHELL_PATH=$WORK/dist/hellish" "HELLISH_VERSION=$ver")
	echo "guest shell: $GUEST (as $ver)"
else
	echo "guest shell: born2root's default (the published hellish release)"
fi

# Every command below is `sh -c '...'` from inside the submodule: that is how
# a person runs it, and it is what makes born2root's probe see the launcher.
EXTRA_ENV=()
run_from() { # run_from <shell> <command string>
	local sh="$1"; shift
	( cd "$B2R" && env -i PATH="$WORK/bin:$(dirname "$H"):$PATH" HOME="$FAKE" \
		USER="$(id -un)" LOGNAME="$(id -un)" TERM=dumb NO_COLOR=1 LC_ALL=C \
		HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
		ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:exitcode=0" LSAN_OPTIONS="exitcode=0" \
		VBOX_USER_HOME="$FAKE/.config/VirtualBox" VBOX_IPC_SOCKETID=born2root-build \
		BORN2ROOT_TRIPWIRE="${BORN2ROOT_TRIPWIRE_LOG:+1}" BORN2ROOT_TRIPWIRE_LOG="${BORN2ROOT_TRIPWIRE_LOG:-}" \
		"${EXTRA_ENV[@]}" "$sh" -c "$*" </dev/null )
}
# "hellish only": docker/Dockerfile.born2root makes /bin/bash log every
# invocation made while BORN2ROOT_TRIPWIRE is set -- run_from sets it, so
# the log holds exactly what born2root and Inception ran under bash during
# this run. Empty is the requirement; anything else names the offender.
tripwire_report() { # tripwire_report <label>
	[ -n "${BORN2ROOT_TRIPWIRE_LOG:-}" ] && [ -f "$BORN2ROOT_TRIPWIRE_LOG" ] || return 0
	if [ ! -s "$BORN2ROOT_TRIPWIRE_LOG" ]; then ok "$1: nothing ran under bash (tripwire log empty)"
	else
		ko "$1: $(wc -l <"$BORN2ROOT_TRIPWIRE_LOG") bash invocation(s) during the run"
		sed 's/^/   bash: /' "$BORN2ROOT_TRIPWIRE_LOG" | head -12
	fi
	: >"$BORN2ROOT_TRIPWIRE_LOG"
}
vbox() { VBOX_USER_HOME="$FAKE/.config/VirtualBox" VBOX_IPC_SOCKETID=born2root-build VBoxManage "$@"; }
vbox_state() { vbox showvminfo "$VM_NAME" --machinereadable 2>/dev/null | awk -F'"' '$1=="VMState="{print $2}'; }
guest() { # guest <cmd...>  -- over the ssh config born2root wrote
	"$WORK/bin/ssh" -o BatchMode=yes -o ConnectTimeout=15 b2b "$@"
}

# ---- 0. the probe must hand the scripts to hellish ------------------------
say "0. which shell does born2root's Makefile pick when hellish launches make?"
picked="$(run_from "$H" 'make -n backend' 2>/dev/null \
	| grep -m1 -oE '[^ ]+ setup/host/select_backend.sh' | cut -d' ' -f1)"
if [ "$picked" = "$H" ]; then ok "SCRIPT_SH = $picked"; else ko "SCRIPT_SH = ${picked:-nothing} (wanted $H)"; exit 1; fi
[ -n "${BORN2ROOT_TRIPWIRE_LOG:-}" ] && : >"$BORN2ROOT_TRIPWIRE_LOG"

# ---- 1. the ISO: hellish's tree == bash's tree ----------------------------
if [ "${BORN2ROOT_SKIP_ISO_PARITY:-0}" != 1 ] && [ "${BORN2ROOT_REUSE_VM:-0}" != 1 ]; then
	say "1. preseeded ISO under bash, then under hellish, trees diffed"
	for sh in bash "$H"; do
		tag="$(basename "$sh")"
		EXTRA_ENV=(ISO_DIR="$WORK/iso_extract.$tag" OUTPUT_ISO="$WORK/preseed.$tag.iso" FORCE_ISO=1)
		t0=$(date +%s)
		run_from "$sh" "make gen_iso ${SHELL_ARGS[*]}" >"$WORK/gen_iso.$tag.log" 2>&1; rc=$?
		echo "   $tag: rc=$rc in $(( $(date +%s) - t0 ))s"
		[ "$rc" = 0 ] || { ko "make gen_iso under $tag (see $WORK/gen_iso.$tag.log)"; tail -5 "$WORK/gen_iso.$tag.log"; }
		rm -rf "$WORK/tree.$tag"; mkdir -p "$WORK/tree.$tag"
		xorriso -osirrox on -indev "$WORK/preseed.$tag.iso" -extract / "$WORK/tree.$tag" >/dev/null 2>&1
		chmod -R u+w "$WORK/tree.$tag"
	done
	EXTRA_ENV=()
	# Same files, same bytes; initrd.gz decompressed since gzip stamps the time.
	( cd "$WORK/tree.bash" && find . -type f | sort ) >"$WORK/list.bash"
	( cd "$WORK/tree.hellish" && find . -type f | sort ) >"$WORK/list.hellish"
	if cmp -s "$WORK/list.bash" "$WORK/list.hellish"; then ok "same file list ($(wc -l <"$WORK/list.bash") files)"
	else ko "file lists differ"; diff "$WORK/list.bash" "$WORK/list.hellish" | head; fi
	# The injected preseed rides in a cpio archive appended to each initrd,
	# and a newc header carries the temp copy's inode, mtime and device --
	# so two builds a minute apart differ in those hex digits under the same
	# shell. Blank those fields of every newc header before comparing;
	# md5sum.txt then differs only in the initrd lines, and each ISO's copy
	# is checked against its own tree instead.
	norm_initrd() {
		zcat "$1" 2>/dev/null | python3 -c '
import re, sys
d = sys.stdin.buffer.read(); out = bytearray(d)
for m in re.finditer(rb"070701[0-9A-Fa-f]{104}", d):
    h = m.start()
    out[h + 6:h + 14] = b"0" * 8        # c_ino
    out[h + 46:h + 54] = b"0" * 8       # c_mtime
    out[h + 62:h + 78] = b"0" * 16      # c_devmajor, c_devminor
sys.stdout.buffer.write(bytes(out))'
	}
	nd=0
	while IFS= read -r f; do
		case "$f" in
		*initrd.gz)
			norm_initrd "$WORK/tree.bash/$f" | cmp -s - <(norm_initrd "$WORK/tree.hellish/$f") \
				|| { nd=$((nd + 1)); echo "   differs: $f (decompressed, cpio header fields masked)"; } ;;
		*md5sum.txt)
			grep -v 'initrd.gz' "$WORK/tree.bash/$f" | cmp -s - <(grep -v 'initrd.gz' "$WORK/tree.hellish/$f") \
				|| { nd=$((nd + 1)); echo "   differs: $f (beyond the initrd lines)"; } ;;
		*) cmp -s "$WORK/tree.bash/$f" "$WORK/tree.hellish/$f" || { nd=$((nd + 1)); echo "   differs: $f"; } ;;
		esac
	done <"$WORK/list.bash"
	if [ "$nd" = 0 ]; then ok "every file byte-identical (custom shell, preseed, boot menus, initrds)"; else ko "$nd file(s) differ between the two ISOs"; fi
	for tag in bash hellish; do
		if ( cd "$WORK/tree.$tag" && md5sum -c --quiet md5sum.txt >/dev/null 2>&1 ); then ok "$tag ISO: md5sum.txt matches its tree"
		else ko "$tag ISO: md5sum.txt does not match its tree"; fi
	done
	[ "$fail" = 0 ] || { echo "   (both trees kept under $WORK for a look)"; exit 1; }
	rm -rf "$WORK/tree.bash" "$WORK/tree.hellish" "$WORK/iso_extract.bash" "$WORK/preseed.bash.iso"
	# the bash-built reference ISO is bash by design; only the hellish build counts
	[ -n "${BORN2ROOT_TRIPWIRE_LOG:-}" ] && : >"$BORN2ROOT_TRIPWIRE_LOG"
fi

# ---- 2..5, once per backend ------------------------------------------------
run_backend() { # run_backend qemu|virtualbox
	local be="$1" VM_PATH LOG rc t0 reuse=0 gsh gver out wrap ilog tmpf quiet st
	VM_PATH="$WORK/vm-$be"; LOG="$WORK/make_all.$be.log"
	mkdir -p "$VM_PATH"
	if [ "${BORN2ROOT_REUSE_VM:-0}" = 1 ]; then
		case "$be" in
		qemu) [ -f "$VM_PATH/$VM_NAME/.installed" ] && reuse=1 ;;
		virtualbox) vbox showvminfo "$VM_NAME" >/dev/null 2>&1 && reuse=1 ;;
		esac
	fi

	# -- 2. make all, from hellish ------------------------------------------
	# QEMU boots whatever ISO it is handed, so it takes the one built above;
	# the VirtualBox installer looks for debian-*-preseed.iso in the repo
	# root, so that path builds it there (born2root's default, gitignored).
	if [ "$reuse" = 1 ]; then
		say "[$be] 2. guest already built: boot it from hellish (log: $LOG)"
		case "$be" in
		qemu) run_from "$H" "make qemu_start VM_PATH=$VM_PATH" >"$LOG" 2>&1; rc=$? ;;
		virtualbox) run_from "$H" "make start_vm VM_PATH=$VM_PATH" >"$LOG" 2>&1; rc=$? ;;
		esac
	else
		say "[$be] 2. make all BACKEND=$be, launched from hellish (log: $LOG)"
		case "$be" in
		qemu) EXTRA_ENV=(ISO_DIR="$WORK/iso_extract.hellish" OUTPUT_ISO="$WORK/preseed.hellish.iso" ISO="$WORK/preseed.hellish.iso") ;;
		virtualbox) EXTRA_ENV=(ISO_DIR="$WORK/iso_extract.hellish") ;;
		esac
		t0=$(date +%s)
		run_from "$H" "make all BACKEND=$be VM_PATH=$VM_PATH ${SHELL_ARGS[*]}" >"$LOG" 2>&1; rc=$?
		EXTRA_ENV=()
		echo "   rc=$rc in $(( ( $(date +%s) - t0 ) / 60 )) min"
	fi
	grep -aE '▶|✓|✗|⚠' "$LOG" | sed 's/\x1b\[[0-9;]*[A-Za-z]//g; s/^/   | /' | tail -40
	if [ "$rc" != 0 ]; then ko "[$be] make exited $rc"; tail -20 "$LOG"; return 1; fi
	ok "[$be] the guest is up"
	grep -qaE 'AddressSanitizer|LeakSanitizer' "$LOG" && ko "[$be] sanitizer report in the log"
	tripwire_report "[$be] make all"

	# -- 3. the guest, over the ssh config born2root wrote --------------------
	say "[$be] 3. the guest answers, and hellish is its login shell"
	if [ ! -s "$FAKE/.ssh/config" ]; then ko "[$be] no ssh config was written"; return 1; fi
	if ! guest true 2>/dev/null; then
		# First boot installs Docker and WordPress before sshd settles; give it time.
		for _ in $(seq 1 60); do sleep 10; guest true 2>/dev/null && break; done
	fi
	guest true 2>/dev/null && ok "ssh b2b answers" || { ko "[$be] ssh b2b never answered"; return 1; }
	gsh="$(guest 'getent passwd $(id -un) | cut -d: -f7' 2>/dev/null)"
	case "$gsh" in */hellish) ok "login shell in the guest: $gsh" ;; *) ko "[$be] login shell in the guest: '$gsh'" ;; esac
	# born2root's guest installs /usr/bin/hellish as a bash wrapper: interactive
	# logins exec /usr/bin/hellish.real, non-interactive ssh commands go to bash
	# (preseeds/b2b-setup.sh, for VS Code's Remote-SSH bootstrap). So the baked
	# binary is asked directly, and the interactive path through a pty.
	gver="$(guest '/usr/bin/hellish.real --version 2>&1 | head -1' 2>/dev/null)"
	case "$gver" in hellish,*) ok "baked binary in the guest: $gver" ;; *) ko "[$be] baked binary in the guest: '$gver'" ;; esac
	out="$(guest '/usr/bin/hellish.real -c "echo \$0; echo \$((6*7)); echo a b | wc -w"' 2>/dev/null | tr '\n' ' ')"
	case "$out" in *hellish*" 42 2 "*) ok "hellish.real -c in the guest: $out" ;; *) ko "[$be] hellish.real -c in the guest: '$out'" ;; esac
	wrap="$(guest 'head -1 /usr/bin/hellish; echo $0' 2>/dev/null | tr '\n' ' ')"
	case "$wrap" in "#!/bin/bash /bin/bash "*) ok "ssh wrapper as born2root installs it (non-interactive -> bash)" ;; *) ko "[$be] ssh wrapper: '$wrap'" ;; esac
	ilog="$(printf 'echo INTERACTIVE-$0-$((6*7))\nexit\n' | "$WORK/bin/ssh" -tt -o BatchMode=yes -o ConnectTimeout=15 b2b 2>/dev/null | tr -d '\r' | grep -o 'INTERACTIVE-/[^[:space:]]*' | head -1)"
	case "$ilog" in *hellish*-42) ok "interactive login runs hellish: $ilog" ;; *) ko "[$be] interactive login: '$ilog'" ;; esac
	tmpf="$WORK/roundtrip.txt"; printf 'born2root %s\n' "$(date +%s)" >"$tmpf"
	if "$WORK/bin/scp" -q "$tmpf" b2b:/tmp/roundtrip.txt 2>/dev/null \
		&& "$WORK/bin/scp" -q b2b:/tmp/roundtrip.txt "$tmpf.back" 2>/dev/null \
		&& cmp -s "$tmpf" "$tmpf.back"; then ok "scp there and back"; else ko "[$be] scp round trip"; fi
	if printf 'ls /tmp/roundtrip.txt\n' | "$WORK/bin/sftp" -q -b - b2b >/dev/null 2>&1; then ok "sftp"; else ko "[$be] sftp"; fi

	# -- 4. the parity table, then the status page, both from hellish --------
	say "[$be] 4. verify_guest and the status page, launched from hellish"
	# First boot keeps provisioning after sshd is up (ufw rules, the hellishrc
	# plugins); the parity table checks those, so wait for it to finish. The
	# bracket keeps pgrep from matching the ssh command that carries the name.
	quiet=0
	for _ in $(seq 1 90); do
		if guest 'pgrep -f "first[-]boot-setup" >/dev/null' 2>/dev/null; then quiet=0; else quiet=$((quiet + 1)); fi
		[ "$quiet" -ge 2 ] && break
		sleep 10
	done
	[ "$quiet" -ge 2 ] && ok "first boot finished" || ko "[$be] first-boot-setup.sh still running after 15 min"
	# ORDER MATTERS: this runs only once first boot has FINISHED (the wait
	# just above). First boot keeps provisioning after sshd answers, and its
	# upstream hellish installer is one of the LATE steps -- refreshing right
	# after `ssh b2b answers` (as this once did, in step 3) won the race for a
	# moment, passed its own size check, and was then clobbered back to the
	# release by first boot before `make inception` baked the shell into the
	# containers. So it sits here: the last hand on hellish.real before 4b.
	# born2root's first boot installs the PUBLISHED hellish release over the
	# ISO-baked binary (setup/install/hellish/install_hellish_upstream.sh runs
	# `curl .../install.sh | sh`). The corpus exists to test THIS tree, so put
	# the binary this run built back as /usr/bin/hellish.real -- exactly what a
	# developer validating a fix does by hand. Without it the guest would run
	# whatever is released, and a bug fixed on the branch (e.g. the escaped
	# backtick in a heredoc that broke Inception's mariadb bootstrap under
	# 2.9.1) would still fail here. sudo wants a tty and a password (requiretty);
	# the password is the dlesieur account password the preseed set.
	if [ -f "$GUEST" ]; then
		gpass="$(awk '$1=="d-i" && $2=="passwd/user-password" {print $4; exit}' "$B2R/preseeds/preseed.cfg" 2>/dev/null)"
		want=$(wc -c <"$GUEST")
		# Replace EVERY hellish.real on the guest, on EVERY mount. The upstream
		# installer (install_hellish_upstream.sh, run with HOME=/home/<user>)
		# scatters copies -- /usr/bin, /usr/local/bin, and ~/.local/bin which the
		# login PATH puts first -- and Inception's `make setup` picks the shell
		# baked into the containers via `command -v hellish.real`, so one missed
		# copy means the images run the released binary (its heredoc escaped-
		# backtick bug then breaks mariadb's bootstrap). GOTCHA: born2root puts
		# /home on its own LVM volume, so `find / -xdev` (as this used to) stays
		# on / and never touches ~/.local/bin/hellish.real -- exactly the copy
		# the deploy's login shell resolves. So find PER MOUNT (an -xdev find
		# rooted at each mountpoint from /proc/mounts) to cover them all, and
		# also swap any ELF binary literally named `hellish` (the real binary,
		# never the #! bash wrapper) so the recipe interpreter is this tree too.
		if [ -n "$gpass" ] && "$WORK/bin/scp" -q "$GUEST" b2b:/tmp/hellish.new 2>/dev/null \
			&& printf '%s\n' "$gpass" | "$WORK/bin/ssh" -tt b2b \
				"sudo -S -p '' sh -c 'n=/tmp/hellish.new; for mp in \$(cut -d\" \" -f2 /proc/mounts | sort -u); do find \"\$mp\" -xdev -name hellish.real -type f 2>/dev/null; done | while IFS= read -r f; do install -m 755 \"\$n\" \"\$f\"; done; install -m 755 \"\$n\" /usr/bin/hellish.real; for mp in \$(cut -d\" \" -f2 /proc/mounts | sort -u); do find \"\$mp\" -xdev -name hellish -type f 2>/dev/null; done | while IFS= read -r f; do head -c4 \"\$f\" | grep -qa ELF && install -m 755 \"\$n\" \"\$f\"; done; rm -f \"\$n\"'" >/dev/null 2>&1; then
			got=$(guest 'command -v hellish.real >/dev/null && wc -c < "$(command -v hellish.real)"' 2>/dev/null | tr -d '\r')
			# also check the login shell's own resolution (its PATH differs).
			# Over `ssh -tt` that shell is interactive, so hellish prints its
			# startup banner+animation before our stdin runs -- and it is out
			# already, so it can't be silenced from stdin. Wrap the size in a
			# marker and grep it, exactly as the ilog check below does; require a
			# digit after `=` so the pty's echo of our own input line (which
			# reads LGOTMARK=$(...)) cannot match.
			lgot=$(printf 'echo "LGOTMARK=$(wc -c < "$(command -v hellish.real)")."\nexit\n' | "$WORK/bin/ssh" -tt -o BatchMode=yes b2b 2>/dev/null | tr -d '\r' | grep -o 'LGOTMARK=[0-9][0-9]*' | head -1 | tr -dc '0-9')
			# and assert EVERY hellish.real is now this tree -- not just the one
			# command -v happens to resolve. The old check trusted a single
			# `command -v` whose non-tty PATH found the refreshed /usr/bin copy
			# and reported a MATCH while a published copy on /home slid into the
			# containers. Distinct sizes across all mounts must be exactly want.
			sizes=$(guest 'for mp in $(cut -d" " -f2 /proc/mounts | sort -u); do find "$mp" -xdev -name hellish.real -type f 2>/dev/null; done | while IFS= read -r f; do wc -c < "$f"; done | sort -u | tr "\n" ","' 2>/dev/null | tr -d '\r')
			if [ "$got" = "$want" ] && [ "$lgot" = "$want" ] && [ "$sizes" = "$want," ]; then
				ok "guest hellish.real refreshed to the binary under test everywhere ($(basename "$GUEST"), $want bytes; command -v, login, and all copies agree)"
			else ko "[$be] refresh incomplete: command -v=$got login=$lgot copies=[$sizes], wanted every hellish.real = $want"; fi
		else
			ko "[$be] could not refresh the guest's hellish.real -- it would run the released binary, not this tree"
		fi
	fi
	run_from "$H" "make verify_guest BACKEND=$be VM_PATH=$VM_PATH" >"$WORK/verify_guest.$be.txt" 2>&1 \
		&& ok "verify_guest: $(grep -oE '[0-9]+/[0-9]+ checks passed' "$WORK/verify_guest.$be.txt" | head -1) -> $WORK/verify_guest.$be.txt" \
		|| { ko "[$be] verify_guest"; grep -E '✗|failed' "$WORK/verify_guest.$be.txt" | head -6 | sed 's/^/   /'; }
	case "$be" in
	qemu) run_from "$H" "make qemu_status VM_PATH=$VM_PATH" >"$WORK/status.$be.txt" 2>&1 && ok "qemu_status" || ko "[$be] qemu_status" ;;
	virtualbox) run_from "$H" "make status VM_PATH=$VM_PATH" >"$WORK/status.$be.txt" 2>&1 && ok "status" || ko "[$be] status" ;;
	esac
	sed 's/\x1b\[[0-9;]*[A-Za-z]//g; s/^/   | /' "$WORK/status.$be.txt" | grep -v '^   | *$' | head -14

	# -- 4b. Inception: deploy from hellish, audit under hellish in the guest -
	if [ "${BORN2ROOT_INCEPTION:-1}" != 0 ] && [ -f "$ROOT/tests/inception/Makefile" ]; then
		say "[$be] 4b. make inception (SRC=tests/inception), launched from hellish"
		t0=$(date +%s)
		run_from "$H" "make inception VM_NAME=$VM_NAME VM_PATH=$VM_PATH SRC=$ROOT/tests/inception" >"$WORK/inception.$be.log" 2>&1; rc=$?
		echo "   rc=$rc in $(( ( $(date +%s) - t0 ) / 60 )) min"
		grep -aE '✓|✗|⚠|→' "$WORK/inception.$be.log" | sed 's/\x1b\[[0-9;]*[A-Za-z]//g; s/^/   | /' | tail -14
		[ "$rc" = 0 ] && ok "make inception finished" || { ko "[$be] make inception exited $rc"; tail -15 "$WORK/inception.$be.log"; }
		# On any Inception trouble, capture what the guest's own containers say:
		# a health/state table plus the logs of the ones that gate the rest, so
		# a CI failure carries its cause instead of just "dependency failed".
		if [ "$rc" != 0 ]; then
			guest 'echo "=== hellish binaries on the guest ==="; echo "command -v hellish.real=$(command -v hellish.real)"; for f in $(find / -xdev -name "hellish.real" 2>/dev/null); do printf "%s  %s bytes  %s\n" "$f" "$(wc -c <"$f")" "$(md5sum "$f" 2>/dev/null | cut -d" " -f1)"; done; echo "srcs/shell/sh: $(md5sum ~/Documents/inception/srcs/shell/sh 2>/dev/null | cut -d" " -f1) ($(wc -c <~/Documents/inception/srcs/shell/sh 2>/dev/null) bytes)"; docker ps -a --format "{{.Names}}\t{{.Status}}"; for c in mariadb wordpress nginx; do echo "=== docker logs $c (tail) ==="; docker logs "$c" 2>&1 | tail -30; echo "=== $c /bin/sh ==="; docker exec "$c" sh -c "readlink -f /bin/sh; md5sum /bin/sh 2>/dev/null; sh --version 2>&1 | head -1" 2>&1; done' \
				>"$WORK/inception_containers.$be.txt" 2>&1 || true
			sed 's/^/   guest: /' "$WORK/inception_containers.$be.txt" | tail -50
		fi
		grep -qa 'the stack builds under /usr/bin/hellish.real' "$WORK/inception.$be.log" \
			&& ok "the guest built the stack under hellish.real" || ko "[$be] the guest did not build under hellish.real"
		# "hellish only" inside the containers too: Inception's make setup
		# copies the static shell it was launched from into its images as
		# /bin/sh (its srcs/shell/README.md), so every entrypoint, healthcheck
		# and `docker exec ... sh` in the guest must answer as hellish.
		out=$(guest 'for c in $(docker ps --format "{{.Names}}"); do printf "%s %s\n" "$c" "$(docker exec "$c" sh -c "sh --version 2>/dev/null | head -1")"; done' 2>/dev/null | tr -d '\r')
		n=$(printf '%s\n' "$out" | grep -c .); h=$(printf '%s\n' "$out" | grep -c 'hellish, version')
		if [ "$n" -gt 0 ] && [ "$n" = "$h" ]; then ok "/bin/sh in all $n containers is $(printf '%s\n' "$out" | head -1 | cut -d' ' -f2-)"
		else ko "[$be] /bin/sh is not hellish in every container ($h of $n)"; printf '%s\n' "$out" | sed 's/^/   /'; fi
		# The compliance suite against the live stack, under hellish.real and
		# under /bin/sh, from the same login-shell path the deploy used. The
		# site is live between the two runs, so byte counts, post counts and
		# backup stamps move; those are masked, every verdict is compared.
		for gsh in /usr/bin/hellish.real /bin/sh; do
			tag=$(basename "$gsh")
			"$WORK/bin/ssh" -tt -o BatchMode=yes b2b "cd ~/Documents/inception && NO_COLOR=1 make test SCRIPT_SH=$gsh" 2>/dev/null \
				| tr -d '\r' | sed 's/\x1b\[[0-9;]*[A-Za-z]//g' >"$WORK/compliance.$be.$tag.txt"
			sed -E 's/[0-9]+ bytes/N bytes/g; s/[0-9]+ posts/N posts/g; s/[0-9]{8}-[0-9]{6}/STAMP/g' \
				"$WORK/compliance.$be.$tag.txt" >"$WORK/compliance.$be.$tag.norm"
			echo "   $tag: $(grep -E 'Summary' "$WORK/compliance.$be.$tag.txt" | tail -1 | sed 's/^ *//')"
		done
		if cmp -s "$WORK/compliance.$be.hellish.real.norm" "$WORK/compliance.$be.sh.norm"; then
			ok "compliance suite: same report under hellish.real and /bin/sh ($(wc -l <"$WORK/compliance.$be.sh.txt") lines)"
		else
			ko "[$be] compliance suite differs between hellish.real and /bin/sh"
			diff "$WORK/compliance.$be.sh.norm" "$WORK/compliance.$be.hellish.real.norm" | head -12 | sed 's/^/   /'
		fi
		# Every verdict about the stack must be a pass. The [P] section judges
		# the repository's submission state (is this tree on the remote's
		# default branch?), which a build from a feature branch fails for
		# reasons that have nothing to do with the shell; it stays visible
		# above, and out of this gate.
		bad=$(awk '/^\[P\]/{p=1} /^\[[A-OQ-Z]\]/{p=0} !p && /✘/' "$WORK/compliance.$be.sh.txt")
		if [ -n "$bad" ]; then
			ko "[$be] compliance suite: failed checks outside [P]"; printf '%s\n' "$bad" | head -8 | sed 's/^/   /'
		else
			ok "compliance suite: every check on the stack passes ([P] excluded: repository state)"
		fi
		run_from "$H" "make verify_access VM_NAME=$VM_NAME" >"$WORK/verify_access.$be.txt" 2>&1 \
			&& ok "verify_access from the host" || { ko "[$be] verify_access"; grep -aE '✗|FAIL' "$WORK/verify_access.$be.txt" | head -6 | sed 's/^/   /'; }
		tripwire_report "[$be] make inception + verify_access"
	fi

	# -- 5. shut it down, from hellish ---------------------------------------
	# host_access left a proxy behind (a detached process here: no systemd
	# --user under a substitute HOME); its own undo is the way to stop it.
	if [ "${BORN2ROOT_INCEPTION:-1}" != 0 ] && [ -f "$ROOT/tests/inception/Makefile" ]; then
		run_from "$H" "make host_access_undo VM_NAME=$VM_NAME" >"$WORK/host_access_undo.$be.txt" 2>&1 \
			&& ok "host_access_undo" || { ko "[$be] host_access_undo"; tail -3 "$WORK/host_access_undo.$be.txt"; }
	fi
	case "$be" in
	qemu)
		say "[$be] 5. make qemu_stop, launched from hellish"
		run_from "$H" "make qemu_stop VM_PATH=$VM_PATH" >"$WORK/stop.$be.txt" 2>&1 && ok "qemu_stop" || { ko "[$be] qemu_stop"; tail -3 "$WORK/stop.$be.txt"; }
		if pid="$(head -n1 "$VM_PATH/$VM_NAME/qemu.pid" 2>/dev/null)" && [ -n "$pid" ] && [ -d "/proc/$pid" ]; then
			ko "[$be] QEMU (pid $pid) still running"
		else
			ok "QEMU is gone"
		fi ;;
	virtualbox)
		say "[$be] 5. make poweroff (ACPI), launched from hellish"
		run_from "$H" "make poweroff VM_PATH=$VM_PATH" >"$WORK/stop.$be.txt" 2>&1 && ok "poweroff" || { ko "[$be] poweroff"; tail -3 "$WORK/stop.$be.txt"; }
		for _ in $(seq 1 60); do st="$(vbox_state)"; [ "$st" = poweroff ] && break; sleep 3; done
		if [ "$st" = poweroff ]; then ok "VM state: poweroff"; else ko "[$be] VM state after 3 min: '$st'"; vbox controlvm "$VM_NAME" poweroff >/dev/null 2>&1; fi
		if [ "$PURGE" = 1 ] && [ "$fail" = 0 ]; then
			run_from "$H" "make rm_disk_image VM_PATH=$VM_PATH" >>"$WORK/stop.$be.txt" 2>&1 && ok "rm_disk_image (unregistered and deleted)" || ko "[$be] rm_disk_image"
		fi ;;
	esac
}

for be in $BACKENDS; do
	run_backend "$be"
	[ "$fail" = 0 ] || break
done

say "$( [ "$fail" = 0 ] && echo "born2root built and verified under hellish ($BACKENDS)" || echo "born2root: FAILURES above" )"
[ "$PURGE" = 1 ] && [ "$fail" = 0 ] && rm -rf "$WORK"
exit "$fail"

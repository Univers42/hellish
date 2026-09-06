#!/usr/bin/env bash
# ============================================================================
# tests/born2root_check.sh -- hellish runs born2root's scripts the way bash does.
#
# born2root (tests/born2root, a submodule) is a real project: ~145 bash
# scripts that build a Debian VM on QEMU or VirtualBox, with hellish baked in
# as the guest's login shell. It is the acceptance corpus for the bash
# dialect -- issues #118 to #122 all came out of running it under hellish.
# Building the VM needs a hypervisor and twenty minutes, so this checks
# everything short of that, against bash:
#
#   1. every *.sh parses under `hellish -n` exactly when it parses under
#      `bash -n` -- same exit status, no side effects, the whole tree;
#   2. born2root's own unit tests (tests/test_*.sh: they stub VBoxManage and
#      qemu, work in mktemp directories and need no root) and the host-side
#      helpers that are safe to run (the help renderer, the backend probe)
#      print the same stdout and exit with the same status under both shells.
#      mktemp names differ per run and are normalised before the diff.
#      hellish's stderr is scanned for sanitizer reports.
#
# hellish is put first on PATH for its side of the run, so born2root's
# Makefile (which runs its scripts with the shell make was launched from)
# picks it for the recipes the unit tests drive -- `make pull` in
# test_make_pull.sh runs under hellish end to end.
#
# Skips cleanly (exit 0, with a notice) when the submodule is not checked out:
#     git submodule update --init tests/born2root
#
# The rest -- `make all` from hellish building the guest for real on KVM --
# is tests/born2root_build.sh (`make born2root-vm`), run by hand or by the
# weekly born2root-vm workflow.
# ============================================================================
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
H="${HELLISH_BIN:-$ROOT/build/bin/hellish}"
B2R="$ROOT/tests/born2root"

if [ ! -f "$B2R/Makefile" ]; then
	echo "born2root: submodule not checked out (git submodule update --init tests/born2root) -- skipping"
	exit 0
fi
if [ ! -x "$H" ]; then
	echo "error: hellish not built at $H (run 'make' first)" >&2
	exit 2
fi

# Same pinned oracle as tests/tester and run_scripts.sh; the scripts are
# #!/bin/bash and use its dialect, so plain `bash`, not --posix.
ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
if [ -x "$ORACLE_HOME/bin/bash" ]; then
	BASH_BIN="$ORACLE_HOME/bin/bash"
else
	BASH_BIN="$(command -v bash)"
fi

OUT="$(mktemp -d)"
FAKE_HOME="$OUT/home"
mkdir -p "$FAKE_HOME"
trap 'rm -rf "$OUT"' EXIT
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:exitcode=0"
export LSAN_OPTIONS="exitcode=0"

fail=0

# ---- 1. parse sweep --------------------------------------------------------
total=0; bad=0
while IFS= read -r f; do
	total=$((total + 1))
	timeout 20 "$BASH_BIN" -n "$f" >/dev/null 2>&1; brc=$?
	timeout 20 "$H" -n "$f" >/dev/null 2>"$OUT/perr"; hrc=$?
	if [ "$brc" != "$hrc" ]; then
		bad=$((bad + 1))
		echo "PARSE MISMATCH  bash=$brc hellish=$hrc  ${f#$B2R/}"
		head -3 "$OUT/perr" | sed 's/^/    /'
	fi
done < <(find "$B2R" -name '*.sh' -type f -not -path '*/.git/*' | sort)
echo "---- parse: $((total - bad)) ok / $bad mismatch (of $total scripts) ----"
[ "$bad" -eq 0 ] || fail=1

# ---- 2. run the self-contained scripts under both, diff stdout + status ----
# One entry per line: script [args]. Everything here has been read: no
# root, no hypervisor, no writes outside mktemp.
cases=(
	"tests/test_host_ports.sh"
	"tests/test_inception_vm_path.sh"
	"tests/test_make_pull.sh"
	"tests/test_qemu_ports.sh"
	"tests/test_qemu_stop.sh"
	"tests/test_vbox_driver.sh"
	"tests/test_vm_disk.sh"
	"tests/test_vm_path.sh"
	"tests/test_vm_unlock.sh"
	"generate/help.sh"
	"setup/host/select_backend.sh auto"
)
run_case() { # run_case <shell> <path-prefix> <script...>
	local sh="$1" prefix="$2"; shift 2
	( cd "$B2R" && env -i PATH="$prefix$PATH" HOME="$FAKE_HOME" TERM=dumb \
		NO_COLOR=1 COLUMNS=80 LC_ALL=C \
		HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
		timeout 180 "$sh" "$@" </dev/null )
}
normalise() { sed -E 's#/tmp/tmp\.[A-Za-z0-9_]+#/tmp/tmp.X#g'; }

ok=0; ko=0
for c in "${cases[@]}"; do
	# shellcheck disable=SC2206  # word-split on purpose: "script args"
	argv=($c)
	run_case "$BASH_BIN" "" "${argv[@]}" 2>/dev/null | normalise >"$OUT/bo"; brc=${PIPESTATUS[0]}
	run_case "$H" "$(dirname "$H"):" "${argv[@]}" 2>"$OUT/he" | normalise >"$OUT/ho"; hrc=${PIPESTATUS[0]}
	leak=""
	grep -qE 'AddressSanitizer|LeakSanitizer' "$OUT/he" && leak="  (sanitizer report in hellish stderr)"
	if [ "$brc" = "$hrc" ] && cmp -s "$OUT/bo" "$OUT/ho" && [ -z "$leak" ]; then
		ok=$((ok + 1)); printf 'ok    %-44s rc=%s\n' "$c" "$brc"
	else
		ko=$((ko + 1)); printf 'FAIL  %-44s bash rc=%s hellish rc=%s%s\n' "$c" "$brc" "$hrc" "$leak"
		diff "$OUT/bo" "$OUT/ho" | head -12 | sed 's/^/    /'
		grep -E 'AddressSanitizer|LeakSanitizer|hellish:' "$OUT/he" | head -4 | sed 's/^/    stderr: /'
	fi
done
# ---- 3. `make -n all` launched FROM each shell ------------------------------
# born2root's Makefile runs its scripts with the shell make was launched from
# (a capability probe picks it); driving the dry run from hellish exercises
# that probe and the Makefile's $(shell ...) evaluations under hellish. The
# plan is identical up to the shell's own path, which is normalised.
for sh in "$BASH_BIN" "$H"; do
	( cd "$B2R" && env -i PATH="$(dirname "$sh"):$PATH" HOME="$FAKE_HOME" TERM=dumb \
		NO_COLOR=1 COLUMNS=80 LC_ALL=C \
		HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
		timeout 180 "$sh" -c 'make -n all' </dev/null ) 2>"$OUT/mk.err" \
		| sed -E -e 's#(/[A-Za-z0-9_./-]+/)?(hellish|bash) (setup|utils|generate)/#$SH \3/#g' \
			-e "s#(/[A-Za-z0-9_./-]+/)?(hellish|bash) -c#\$SH -c#g" \
			-e 's#/tmp/tmp\.[A-Za-z0-9_]+#/tmp/tmp.X#g' >"$OUT/mk.$(basename "$sh")"
	echo "${PIPESTATUS[0]}" >"$OUT/mk.$(basename "$sh").rc"
done
bn="$(basename "$BASH_BIN")"; hn="$(basename "$H")"
# The probe must have picked hellish when hellish launched make: that is
# the whole point of born2root's SCRIPT_SH, and a hellish the probe rejects
# would silently hand every script back to bash.
picked="$(cd "$B2R" && env -i PATH="$(dirname "$H"):$PATH" HOME="$FAKE_HOME" TERM=dumb \
	HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
	timeout 60 "$H" -c 'make -n all' 2>/dev/null </dev/null \
	| grep -m1 -oE '[^ ]+ setup/host/select_backend.sh' | cut -d' ' -f1)"
if [ "$picked" != "$H" ]; then
	ko=$((ko + 1)); fail=1
	printf 'FAIL  %-44s probe picked %s\n' "make picks hellish as SCRIPT_SH" "${picked:-nothing}"
else
	ok=$((ok + 1)); printf 'ok    %-44s\n' "make picks hellish as SCRIPT_SH"
fi
if cmp -s "$OUT/mk.$bn" "$OUT/mk.$hn" && cmp -s "$OUT/mk.$bn.rc" "$OUT/mk.$hn.rc"; then
	ok=$((ok + 1))
	printf 'ok    %-44s rc=%s (%s lines)\n' "make -n all, launched from the shell" \
		"$(cat "$OUT/mk.$hn.rc")" "$(wc -l <"$OUT/mk.$hn")"
else
	ko=$((ko + 1))
	printf 'FAIL  %-44s bash rc=%s hellish rc=%s\n' "make -n all, launched from the shell" \
		"$(cat "$OUT/mk.$bn.rc")" "$(cat "$OUT/mk.$hn.rc")"
	diff "$OUT/mk.$bn" "$OUT/mk.$hn" | head -12 | sed 's/^/    /'
	head -4 "$OUT/mk.err" | sed 's/^/    stderr: /'
fi
echo "---- run: $ok ok / $ko FAIL (of $((${#cases[@]} + 2)) cases vs $("$BASH_BIN" --version | head -1 | sed 's/.*version \([0-9.]*\).*/bash \1/')) ----"
[ "$ko" -eq 0 ] || fail=1
exit "$fail"

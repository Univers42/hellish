#!/usr/bin/env bash
# ============================================================================
# tests/born2root_shell_audit.sh -- which shell ran each script of a born2root
# run? Every one of them, and nothing else, must be hellish.
#
# born2root's Makefile runs its scripts with the shell make was launched from
# and, since SHELL := $(SCRIPT_SH), its recipe lines too. tests/born2root_check.sh
# shows the scripts *behave* like bash; this shows *what interpreted them*,
# from the outside, so the claim "launched from hellish, born2root is hellish
# throughout" rests on an exec log rather than on the Makefile's word:
#
#   1. static: every *.sh in the tree declares `#!/usr/bin/env hellish`, and
#      the interpreter names left in the sources (`sh -c`, `/bin/bash`, ...)
#      are exactly the ones listed below with a reason -- a new one fails;
#   2. dynamic: the host-side targets that need no VM are run from hellish
#      with tests/tools/execlog.c preloaded, which records every exec the
#      run performs (path, argv, and the shebang of the file the kernel will
#      open). Each script that ran is listed with its interpreter; a bash,
#      sh, dash or zsh anywhere in the tree of processes fails the run --
#      except the launcher's own oracle side (see 3);
#   3. both shells: the same targets are run from bash, and the set of
#      scripts each run executed must be the same. That is the "verified on
#      both" half: not two different pipelines, one pipeline under two shells.
#
# Usage:
#   tests/born2root_shell_audit.sh                 the default targets
#   tests/born2root_shell_audit.sh inception VM_NAME=debian ...
#                                                  one make invocation of
#                                                  your own (needs the VM),
#                                                  audited the same way
# Env: HELLISH_BIN (build/bin/hellish), HELLISH_ORACLE (~/bash-5.3.9),
#      BORN2ROOT_DIR (another born2root checkout, e.g. the one that knows
#      your VM), AUDIT_KEEP=1 keeps the logs and prints their location.
# With an invocation of your own the two launchers' outputs are shown but
# not required to match: a deploy prints timings and docker's build log.
#
# Static binaries (a guest's hellish.real) and setuid programs (sudo) do not
# load the preload; what they exec is invisible here. The guest side is
# checked by born2root's own `make verify_guest` (interpreter rows) and by
# tests/born2root_build.sh.
# Skips cleanly (exit 0, with a notice) when the submodule is not checked out.
# ============================================================================
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
H="${HELLISH_BIN:-$ROOT/build/bin/hellish}"
B2R="${BORN2ROOT_DIR:-$ROOT/tests/born2root}"
SO="$ROOT/build/execlog.so"

if [ ! -f "$B2R/Makefile" ]; then
	echo "born2root: submodule not checked out (git submodule update --init tests/born2root) -- skipping"
	exit 0
fi
[ -x "$H" ] || { echo "error: hellish not built at $H (run 'make' first)" >&2; exit 2; }
command -v cc >/dev/null 2>&1 || { echo "error: no C compiler for tests/tools/execlog*.c" >&2; exit 2; }
mkdir -p "$ROOT/build"
cc -shared -fPIC -O2 -o "$SO" "$ROOT"/tests/tools/execlog*.c -ldl || exit 2

ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
if [ -x "$ORACLE_HOME/bin/bash" ]; then BASH_BIN="$ORACLE_HOME/bin/bash"; else BASH_BIN="$(command -v bash)"; fi

OUT="$(mktemp -d)"
[ -n "${AUDIT_KEEP:-}" ] || trap 'rm -rf "$OUT"' EXIT
ok=0; ko=0
pass() { ok=$((ok + 1)); printf 'ok    %s\n' "$1"; }
fail() { ko=$((ko + 1)); printf 'FAIL  %s\n' "$1"; }

# ---- 1. static -------------------------------------------------------------
# Interpreter names that may stay in the sources, one regex per line with the
# reason. Anything else that names bash, sh, dash or zsh as an interpreter is
# a regression.
ALLOW='
docker/(Makefile|test_container\.sh): -- the debian-shell-lab: a bash-vs-hellish container, bash is the subject
preseeds/preseed\.cfg:.*early_command -- d-i itself, before /target exists: busybox sh, no guest yet
preseeds/preseed\.cfg:.*finish-install\.d -- same: a d-i finish-install hook
preseeds/preseed\.cfg:.*in-target /bin/bash -- the repository copy; the ISO copy is rewritten to the baked shell (create_custom_iso.sh)
preseeds/preseed\.cfg:.*vbox-poweroff\.sh -- same: rewritten in the ISO copy
preseeds/b2b-setup\.sh:.*B2B_GUEST_SH=/bin/bash -- the default for an ISO built with no shell
preseeds/b2b-setup\.sh:.*usermod -s /bin/bash -- the guard'"'"'s last resort: a login shell that exists beats none
preseeds/b2b-setup\.sh:.*fell back to /bin/bash -- its log line
preseeds/first-boot-setup\.sh:.*B2B_SH=/bin/bash -- same default
setup/host/provision_vm\.sh:.*echo /bin/sh -- askpass helper on a guest that has no hellish.real
setup/host/deploy_inception\.sh:.*echo sh -- /etc/hosts line on a guest that has no hellish.real
setup/host/verify_guest_parity\.sh:.*root shell -- root keeps /bin/bash: the recovery account
setup/install/hellish/install_hellish_upstream\.sh:.*(grep -q .\^#!/bin/sh|printf .sh.|/bin/bash /root/first-boot) -- validates upstream'"'"'s installer, converts an old guest
setup/install/install_nodejs\.sh:.*\| bash -- nvm'"'"'s installer, run the way nvm documents
setup/install/install_zsh\.sh:.*sh -c -- oh-my-zsh'"'"'s installer, run the way it documents
setup/b2br_internal/b2b_setup\.sh:.*useradd -- a new account'"'"'s shell is policy, not a script interpreter
diagnostic/b2b_learling_lab\.sh:.*useradd -- same
fixes/fix_policy\.sh:.*grep "/bin/bash" -- reads /etc/passwd
tests/test_vbox_driver\.sh:.*#!/bin/sh -- stub VirtualBoxVM files whose mode bits are the subject
generate/create_custom_iso\.sh:.*(sed -i -e|-e "s\|echo) -- the rewrite rules themselves: what the ISO'"'"'s preseed says instead of /bin/bash and #!/bin/sh
'
shebangs=$(find "$B2R" -name '*.sh' -type f -not -path '*/.git/*' -exec head -q -n1 {} + | grep -c -v '^#!/usr/bin/env hellish$')
total=$(find "$B2R" -name '*.sh' -type f -not -path '*/.git/*' | wc -l)
if [ "$shebangs" = 0 ]; then pass "all $total *.sh declare #!/usr/bin/env hellish"; else
	fail "$shebangs of $total *.sh do not declare #!/usr/bin/env hellish"
	find "$B2R" -name '*.sh' -type f -not -path '*/.git/*' | while read -r f; do
		head -1 "$f" | grep -q '^#!/usr/bin/env hellish$' || echo "        ${f#$B2R/}: $(head -1 "$f")"; done | head -20
fi
grep -rn -E '(^|[^A-Za-z_/.-])(/bin/bash|/usr/bin/env bash|bash -c|\| bash\b|\bbash [a-zA-Z_./$"]+\.sh|sh -c|/bin/sh\b|exec bash|sudo bash|sudo sh\b|\| sh\b|useradd .*-s /bin)' \
	--include='*.sh' --include='*.cfg' --include='Makefile' "$B2R" \
	| sed "s|^$B2R/||" | grep -v -E '^(doc|evals42|vendor)/' | grep -v -E '^[^:]+:[0-9]+:[[:space:]]*#' > "$OUT/refs"
: > "$OUT/refs.bad"
while IFS= read -r line; do
	allowed=0
	while IFS= read -r rule; do
		rule="${rule%% -- *}"; [ -n "$rule" ] || continue
		printf '%s\n' "$line" | grep -qE "^$rule" && { allowed=1; break; }
	done <<< "$ALLOW"
	[ "$allowed" = 1 ] || printf '%s\n' "$line" >> "$OUT/refs.bad"
done < "$OUT/refs"
if [ ! -s "$OUT/refs.bad" ]; then pass "$(wc -l < "$OUT/refs") interpreter names left in the sources, each one listed with its reason"; else
	fail "interpreter names in the sources that are not accounted for ($(wc -l < "$OUT/refs.bad")):"; cut -c1-150 "$OUT/refs.bad" | sed 's/^/        /' | head -30
fi

# ---- 2 + 3. dynamic --------------------------------------------------------
# run <label> <shell> <path-prefix> <make args...>: launch make from <shell>,
# the exec logger preloaded, and summarise what interpreted what.
run() {
	local label="$1" sh="$2" pre="$3"; shift 3
	local log="$OUT/$label.exec"
	# An ASan build of hellish (CI's debug job) refuses to start with a
	# non-ASan library preloaded unless told otherwise; leaks are not this
	# check's subject.
	( cd "$B2R" && PATH="$pre$PATH" HOME="$RUN_HOME" NO_COLOR=1 LD_PRELOAD="$SO" EXECLOG="$log" \
		ASAN_OPTIONS="verify_asan_link_order=0:detect_leaks=0${ASAN_OPTIONS:+:$ASAN_OPTIONS}" \
		"$sh" -c "make $*; echo rc=\$?" > "$OUT/$label.out" 2>&1 )
	[ -s "$log" ] || { : > "$log"; printf 'warn  %s: no exec was logged -- did %s start at all? first lines of its output:\n' "$label" "$sh"; head -3 "$OUT/$label.out" | sed 's/^/        /'; }
	# tests/tools/execlog_classify.py: one line per exec -- pid, caller image, path, shebang, argv. The
	# interpreter of a file is what its shebang says (env X -> X), else the file
	# itself. A shell is an offender when born2root's own code started it: a
	# file under the corpus interpreted by bash/sh/dash/zsh, or hellish/make
	# (the recipes, the scripts) exec'ing one by name. A third-party program
	# that is itself a sh script (VBoxManage, code) or spawns one is reported
	# as foreign, not charged to born2root.
	python3 "$ROOT/tests/tools/execlog_classify.py" "$log" "$OUT/$label" "$B2R" "$sh"
}

mkdir -p "$OUT/home"
# The default targets run under a throwaway HOME so nothing of yours is
# touched. An invocation of your own is a real one (a deploy writes the
# browser CA, the proxy's user unit) and runs with your HOME, as you do.
if [ "$#" -gt 0 ]; then
	TARGETS=("$*"); STRICT_OUT=0; RUN_HOME="$HOME"
else
	TARGETS=("help" "status" "backend BACKEND=auto" "check_system" "-n all" "list_vms" "qemu_status" "qemu_list"); STRICT_OUT=1; RUN_HOME="$OUT/home"
fi
for t in "${TARGETS[@]}"; do
	n=$(printf '%s' "$t" | tr -c 'A-Za-z0-9' '_')
	run "h.$n" "$H" "$(dirname "$H"):" $t
	run "b.$n" "$BASH_BIN" "$(dirname "$BASH_BIN"):" $t
	hs=$(cut -f1 "$OUT/h.$n.scripts" | sort -u); bs=$(cut -f1 "$OUT/b.$n.scripts" | sort -u)
	nsc=$(printf '%s\n' "$hs" | grep -c .)
	nex=$(awk -F'\t' '$1=="hellish"{print $2}' "$OUT/h.$n.interp"); nex=${nex:-0}
	# From hellish: every script under hellish, and no shell started by
	# born2root's own code -- not by a recipe, not by a script.
	if [ -s "$OUT/h.$n.offenders" ]; then
		fail "make $t from hellish: $(wc -l < "$OUT/h.$n.offenders") shell(s) started by born2root itself"
		cut -c1-150 "$OUT/h.$n.offenders" | sed 's/^/        /' | head -8
	elif awk -F'\t' '$2 != "hellish" {bad = 1} END {exit !bad}' "$OUT/h.$n.scripts"; then
		fail "make $t from hellish: a script ran under something else"; sed 's/^/        /' "$OUT/h.$n.scripts" | head -8
	else
		pass "make $t from hellish: $nsc script(s) and $nex hellish exec(s), no bash/sh/dash started by born2root"
	fi
	[ -s "$OUT/h.$n.foreign" ] && printf '      foreign (third-party programs that are or start sh): %s\n' "$(cut -f1 "$OUT/h.$n.foreign" | sort -u | tr '\n' ' ')"
	# From bash: the same scripts, all under bash.
	if [ "$hs" != "$bs" ]; then
		fail "make $t: the two launchers ran different scripts"
		diff <(printf '%s\n' "$bs") <(printf '%s\n' "$hs") | sed 's/^/        /' | head -8
	elif awk -F'\t' '$2 != "bash" {bad = 1} END {exit !bad}' "$OUT/b.$n.scripts"; then
		fail "make $t from bash: a script ran under something other than bash"; sed 's/^/        /' "$OUT/b.$n.scripts" | head -8
	else
		pass "make $t from bash: the same $nsc script(s), every one under bash"
	fi
	# And the same output (the shell's own path masked).
	for s in h b; do sed -e "s|$H|SHELL|g; s|$BASH_BIN|SHELL|g; s|/usr/bin/bash|SHELL|g; s|/bin/bash|SHELL|g" "$OUT/$s.$n.out" > "$OUT/$s.$n.norm"; done
	if cmp -s "$OUT/h.$n.norm" "$OUT/b.$n.norm"; then pass "make $t: same output from both launchers ($(wc -l < "$OUT/h.$n.norm") lines)"
	elif [ "$STRICT_OUT" = 1 ]; then
		fail "make $t: output differs by launcher"; diff "$OUT/b.$n.norm" "$OUT/h.$n.norm" | head -8 | sed 's/^/        /'
	else
		printf 'info  make %s: exit %s from hellish, %s from bash; %s/%s output lines (not compared for a custom invocation)\n' \
			"$t" "$(tail -1 "$OUT/h.$n.out" | sed -n 's/^rc=//p')" "$(tail -1 "$OUT/b.$n.out" | sed -n 's/^rc=//p')" "$(wc -l < "$OUT/h.$n.norm")" "$(wc -l < "$OUT/b.$n.norm")"
	fi
done

# The inventory, once, for the reader: which scripts ran, under what.
printf '\n%s\n' "born2root files executed by the hellish-launched runs, with their interpreter:"
cat "$OUT"/h.*.scripts 2>/dev/null | sort -u | sed 's/^/   /'
printf '%s\n' "every image exec'd by those runs, with a count:"
cat "$OUT"/h.*.interp 2>/dev/null | awk -F'\t' '{c[$1]+=$2} END {for (k in c) printf "   %-28s %d\n", k, c[k]}' | sort
printf '\n%d ok, %d failed' "$ok" "$ko"
[ -n "${AUDIT_KEEP:-}" ] && printf '   (logs in %s)' "$OUT"
printf '\n'
[ "$ko" = 0 ]

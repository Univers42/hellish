#!/usr/bin/env bash
# ============================================================================
# tests/pty_suite.sh -- run EVERY pty/regression test in tests/*.py.
#
# Why this exists, and why it discovers rather than lists:
#
# Each of these files was written to pin down a specific bug so it could not
# come back. That only holds if the file actually RUNS. Every one of them was
# wired into the Makefile and into CI by hand, one target and one CI line per
# file -- and a hand-maintained list is a list that drifts. It had:
# completion_posix_test.py sat in tests/ with no target and no CI job, so the
# POSIX command-search fix it guards was unprotected from the day it landed.
# (The golden suite has the same failure mode, documented at the top of
# tests/tester: a category file not added to test_lists silently never runs.)
#
# So this runner takes no list. It globs tests/*.py, runs all of them, and a
# new regression test is covered the moment it is added -- nobody has to
# remember a second step. `make pty-test` runs this; CI runs `make pty-test`.
#
#   tests/pty_suite.sh                     # everything, against build/bin/hellish
#   tests/pty_suite.sh -- history_opts     # only files matching a pattern
#   HELLISH=/path/to/shell tests/pty_suite.sh
#
# Exit status is the number of failed FILES, capped at 125 (0 == all green).
# ============================================================================
set -u
cd "$(dirname "$0")/.."

# ---- environment isolation -------------------------------------------------
# These tests spawn INTERACTIVE shells, which source ~/.hellishrc. On the
# machine of anyone who actually uses hellish -- this project's whole audience
# -- that loads their real config into the shell under test, and prompt tests
# then diff against someone's personal PS1. Observed: 4 spurious failures with
# a real $HOME and 0 with a clean one, plus the developer's own framework
# writing state mid-run.
#
# HOME is redirected to a throwaway dir rather than unset, because tools that
# expand ~ must still work. The oracle is symlinked in: history_multiline_matrix
# resolves it as ~/bash-5.3.9, so isolating HOME alone would hide it and the
# test would exit 2 without running.
if [ "${PTY_SUITE_KEEP_HOME:-0}" != "1" ]; then
	_iso_home="$(mktemp -d)"
	[ -d "$HOME/bash-5.3.9" ] && ln -s "$HOME/bash-5.3.9" "$_iso_home/bash-5.3.9"
	[ -n "${HELLISH_ORACLE:-}" ] || \
		{ [ -x "$HOME/bash-5.3.9/bin/bash" ] && export HELLISH_ORACLE="$HOME/bash-5.3.9/bin/bash"; }
	HOME="$_iso_home"
	export HOME
	trap 'rm -rf "$_iso_home"' EXIT
fi
unset HISTTIMEFORMAT HISTCONTROL HISTSIZE HISTFILE HISTFILESIZE HISTIGNORE
unset ENV BASH_ENV CDPATH GLOBIGNORE

SHELL_BIN="${HELLISH:-build/bin/hellish}"
[ -x "$SHELL_BIN" ] || { echo "error: $SHELL_BIN is not executable -- run make" >&2; exit 2; }

# Optional filter: tests/pty_suite.sh -- <pattern>
pattern=""
[ "${1:-}" = "--" ] && pattern="${2:-}"

# A file is skipped here ONLY when running it here would be misleading, and
# each skip names where it does run. PTY_SUITE_ALL=1 overrides all of them.
#
# This list is the one hand-maintained thing left, so it is kept to the cases
# that genuinely cannot run in this job, with the reason attached -- a skip
# without a reason is just a test nobody runs.
skip_reason() {
	# This one is skipped BEFORE the PTY_SUITE_ALL override, and stays
	# skipped through it. Every other skip here is about a test being
	# uninformative in this job; this one would run `make my_shell` on the
	# machine invoking it and change the caller's LOGIN SHELL. An override
	# flag meaning "run the ones that are merely unhelpful" must not also
	# mean "rewrite my /etc/passwd entry". It runs as `make my-shell-test`,
	# in a container built for it.
	if [ "$1" = "my_shell_update_test.py" ]; then
		echo "installs a login shell; runs as \`make my-shell-test\`"\
		     "in docker/Dockerfile.my-shell, never on a host."
		return 0
	fi
	[ "${PTY_SUITE_ALL:-0}" = "1" ] && return 1
	case "$1" in
		completion_test.py)
			echo "needs a SAFE=0 (ft_malloc) build to mean anything; a"\
			     "cross-heap free cannot exist on SAFE=1. Runs as"\
			     "\`make completion-test\`, which builds one." ;;
		prompt_latency_test.py)
			echo "measures TIME against bash, and this job builds with"\
			     "ASan -- which slows hellish ~4x and bash not at all,"\
			     "so the ratio would grade the sanitizer. Runs as"\
			     "\`make prompt-latency-test\`, which builds OPT=1." ;;
		update_test.py|update_ui_test.py|net_redir_test.py)
			echo "brings up its own local peer; runs in the \`update\` job"\
			     "as \`make update-test\` / \`make net-redir-test\`." ;;
		*) return 1 ;;
	esac
	return 0
}

# Some files need more than the default wall-clock budget: they drive a real
# terminal and deliberately wait on timing (an idle prompt repaint, a stalled
# git probe, a 400KB flood). A per-file timeout keeps one wedged pty from
# hanging the whole run -- without it a regression that WEDGES the shell looks
# like an infrastructure hang instead of the failure it is.
timeout_for() {
	case "$1" in
		update_test.py|completion_test.py|bg_tty_test.py) echo 900 ;;
		*) echo 420 ;;
	esac
}

# ---- what must NOT share the machine ---------------------------------------
# Most of these files only need a terminal, and a terminal is per-process --
# so they run in parallel and the suite finishes in the time of its longest
# file instead of the sum of all of them (2100s -> ~400s here).
#
# These do not. Each one grades a NUMBER that other load moves: a syscall
# count per keystroke, a time ratio against bash, a repaint that must arrive
# within a deadline, or -- prompt_atomic's second phase -- a reader starved on
# purpose, which every other test on the box starves further. Running them
# alongside 15 siblings does not find bugs, it invents them, which is the
# failure mode this suite has been paying for all week. They go last, alone.
is_serial() {
	case "$1" in
		frontend_budget_test.py|prompt_syscall_budget_test.py) return 0 ;;
		prompt_atomic_test.py|prompt_latency_test.py) return 0 ;;
		git_prompt_stall_test.py|prompt_shortwrite_test.py) return 0 ;;
		parse_scaling_test.py|pattern_cost_test.py) return 0 ;;
		trim_literal_perf_test.py|func_registry_perf_test.py) return 0 ;;
		nonblock_tty_test.py|prompt_resize_test.py) return 0 ;;
		*) return 1 ;;
	esac
}

# One file: run it, park the verdict. Printing happens in the parent so two
# workers cannot interleave half a line each.
run_one() {
	_b="$1"
	_t=$(timeout_for "$_b")
	_s=$(date +%s)
	if timeout "$_t" python3 "tests/$_b" "$SHELL_BIN" \
			> "$RUNDIR/$_b.log" 2>&1; then
		printf 'ok %s\n' "$(( $(date +%s) - _s ))" > "$RUNDIR/$_b.res"
	else
		printf '%s %s\n' "$?" "$(( $(date +%s) - _s ))" > "$RUNDIR/$_b.res"
	fi
}
export -f run_one timeout_for
export SHELL_BIN RUNDIR

report_one() {
	_b="$1"
	read -r _rc _secs < "$RUNDIR/$_b.res"
	if [ "$_rc" = ok ]; then
		printf '\033[32m  ✓ %s\033[0m (%ss)\n' "$_b" "$_secs"
		pass=$((pass + 1))
		rm -f "$RUNDIR/$_b.log" "$RUNDIR/$_b.res"
		return 0
	fi
	printf '\033[31m  ✗ %s\033[0m (%ss, exit %s)\n' "$_b" "$_secs" "$_rc"
	[ "$_rc" = 124 ] && printf '    TIMED OUT after %ss -- a wedged shell counts as a failure\n' "$(timeout_for "$_b")"
	sed -n '/FAIL/p' "$RUNDIR/$_b.log" | head -12 | sed 's/^/    /'
	tail -40 "$RUNDIR/$_b.log" | sed 's/^/    | /'
	fail=$((fail + 1)); failed_files="$failed_files $_b"
	rm -f "$RUNDIR/$_b.log" "$RUNDIR/$_b.res"
}

pass=0; fail=0; skip=0; failed_files=""
start_all=$(date +%s)
RUNDIR="$(mktemp -d)"
export RUNDIR
trap 'rm -rf "$RUNDIR"' EXIT

# One worker per core by default. PTY_JOBS=1 restores the old serial run,
# which is what to reach for when a result looks like contention.
jobs_n="${PTY_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
[ "$jobs_n" -lt 1 ] 2>/dev/null && jobs_n=1
[ "$jobs_n" -gt 16 ] 2>/dev/null && jobs_n=16

printf '\n\033[1m═══ pty / regression suite ═══\033[0m\n'
printf '  shell: %s\n' "$SHELL_BIN"
printf '  jobs:  %s parallel, timing-sensitive files serial\n\n' "$jobs_n"

par=""; ser=""
for f in tests/*.py; do
	b=$(basename "$f")
	if [ -n "$pattern" ] && ! printf '%s' "$b" | grep -q "$pattern"; then
		skip=$((skip + 1)); continue
	fi
	if reason=$(skip_reason "$b"); then
		printf '\033[33m  ~ %s\033[0m skipped: %s\n' "$b" "$reason"
		skip=$((skip + 1)); continue
	fi
	if is_serial "$b"; then ser="$ser $b"; else par="$par $b"; fi
done

if [ -n "$par" ]; then
	printf '\033[1;36m▸ %s files in parallel\033[0m\n' "$(printf '%s\n' $par | wc -l)"
	printf '%s\n' $par | xargs -P "$jobs_n" -I{} bash -c 'run_one "$@"' _ {}
	for b in $par; do report_one "$b"; done
fi
if [ -n "$ser" ]; then
	printf '\033[1;36m▸ %s files serially (timing-sensitive)\033[0m\n' "$(printf '%s\n' $ser | wc -l)"
	for b in $ser; do run_one "$b"; report_one "$b"; done
fi

printf '\n\033[1m═══ %d ok / %d failed' "$pass" "$fail"
[ "$skip" -gt 0 ] && printf ' / %d skipped' "$skip"
printf '  (%ss) ═══\033[0m\n' "$(( $(date +%s) - start_all ))"
if [ "$fail" -ne 0 ]; then
	printf '\033[31mfailed:%s\033[0m\n' "$failed_files"
	[ "$fail" -gt 125 ] && exit 125
	exit "$fail"
fi
exit 0

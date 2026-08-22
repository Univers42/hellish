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
	[ "${PTY_SUITE_ALL:-0}" = "1" ] && return 1
	case "$1" in
		completion_test.py)
			echo "needs a SAFE=0 (ft_malloc) build to mean anything; a"\
			     "cross-heap free cannot exist on SAFE=1. Runs as"\
			     "\`make completion-test\`, which builds one." ;;
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

pass=0; fail=0; skip=0; failed_files=""
start_all=$(date +%s)

printf '\n\033[1m═══ pty / regression suite ═══\033[0m\n'
printf '  shell: %s\n\n' "$SHELL_BIN"

for f in tests/*.py; do
	b=$(basename "$f")
	if [ -n "$pattern" ] && ! printf '%s' "$b" | grep -q "$pattern"; then
		skip=$((skip + 1)); continue
	fi
	if reason=$(skip_reason "$b"); then
		printf '\033[33m  ~ %s\033[0m skipped: %s\n' "$b" "$reason"
		skip=$((skip + 1)); continue
	fi
	t=$(timeout_for "$b")
	printf '\033[1;36m▸ %s\033[0m\n' "$b"
	start=$(date +%s)
	# Every one of these takes the shell path as argv[1] and nothing else;
	# that uniformity is what makes discovery possible, so keep it.
	if timeout "$t" python3 "$f" "$SHELL_BIN" > "/tmp/pty_$$_$b.log" 2>&1; then
		printf '\033[32m  ✓ %s\033[0m (%ss)\n' "$b" "$(( $(date +%s) - start ))"
		pass=$((pass + 1))
	else
		rc=$?
		printf '\033[31m  ✗ %s\033[0m (%ss, exit %s)\n' \
			"$b" "$(( $(date +%s) - start ))" "$rc"
		[ "$rc" = 124 ] && printf '    TIMED OUT after %ss -- a wedged shell counts as a failure\n' "$t"
		sed -n '/FAIL/p' "/tmp/pty_$$_$b.log" | head -12 | sed 's/^/    /'
		tail -4 "/tmp/pty_$$_$b.log" | sed 's/^/    | /'
		fail=$((fail + 1)); failed_files="$failed_files $b"
	fi
	rm -f "/tmp/pty_$$_$b.log"
done

printf '\n\033[1m═══ %d ok / %d failed' "$pass" "$fail"
[ "$skip" -gt 0 ] && printf ' / %d skipped' "$skip"
printf '  (%ss) ═══\033[0m\n' "$(( $(date +%s) - start_all ))"
if [ "$fail" -ne 0 ]; then
	printf '\033[31mfailed:%s\033[0m\n' "$failed_files"
	[ "$fail" -gt 125 ] && exit 125
	exit "$fail"
fi
exit 0

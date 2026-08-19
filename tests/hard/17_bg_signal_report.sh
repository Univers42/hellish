#!/bin/sh
# Issue #17: a background job killed by a signal must be REPORTED, in both of
# the voices bash uses -- the `jobs` status column, and the unsolicited stderr
# line a SCRIPT gets the moment the shell notices the death.
#
# This lives in tests/hard rather than the golden suite for a reason: the
# golden suite drives everything through `-c`, and bash deliberately says
# nothing under -c. Only a script (or piped input) gets the async line, and
# run.sh runs these files as scripts.
#
# The shell's own stderr is captured to a file and replayed on stdout with the
# volatile parts normalised away:
#   - the pid, which changes every run;
#   - the "line N", because hellish's script line numbers are a KNOWN separate
#     defect (update_ctx reports the end of the input batch rather than the
#     running command) -- recorded in backlog.md. The message text, its stream,
#     its ordering relative to stdout, and its field widths are still diffed.
#
# The stderr capture uses ONE redirection per `exec` on purpose: `exec` with
# two redirections on one line is itself broken in hellish (also in backlog),
# and a test must not depend on the bug it is not testing. Each section ends
# with `wait` for the same reason: a job that `jobs` reported but nobody
# waited for is retired by bash and kept by hellish, which is a job-NUMBER
# defect (backlog) and not this test's subject.

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

norm() {
	sed -e 's/^.*: line [0-9][0-9]*: [0-9][0-9]*  */PREFIX /' "$1"
}

echo "== SIGKILL: announced, then retired =="
exec 3>&2
exec 2>"$work/e1"
sleep 9 &
k=$!
sleep 0.4
kill -9 "$k" 2>/dev/null
sleep 0.6
echo "after"
jobs
echo "end"
wait 2>/dev/null
exec 2>&3
exec 3>&-
norm "$work/e1"

echo "== SIGTERM: silent, but jobs still names it =="
exec 3>&2
exec 2>"$work/e2"
sleep 8 &
t=$!
sleep 0.4
kill -TERM "$t" 2>/dev/null
sleep 0.6
echo "after"
jobs
echo "end"
wait 2>/dev/null
exec 2>&3
exec 3>&-
norm "$work/e2"

echo "== SIGHUP: announced =="
exec 3>&2
exec 2>"$work/e3"
sleep 7 &
h=$!
sleep 0.4
kill -HUP "$h" 2>/dev/null
sleep 0.6
echo "after"
jobs
echo "end"
wait 2>/dev/null
exec 2>&3
exec 3>&-
norm "$work/e3"

echo "== normal exit: silent in both voices =="
exec 3>&2
exec 2>"$work/e4"
sleep 0.2 &
sleep 0.7
echo "after"
jobs
echo "end"
wait 2>/dev/null
exec 2>&3
exec 3>&-
norm "$work/e4"

echo "== SIGPIPE: suppressed like TERM =="
exec 3>&2
exec 2>"$work/e5"
sleep 6 &
q=$!
sleep 0.4
kill -PIPE "$q" 2>/dev/null
sleep 0.6
echo "after"
jobs
echo "end"
wait 2>/dev/null
exec 2>&3
exec 3>&-
norm "$work/e5"

echo "== wait reports the signal status =="
sleep 5 &
w=$!
sleep 0.4
kill -9 "$w" 2>/dev/null
wait "$w" 2>/dev/null
echo "wait_status=$?"

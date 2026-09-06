#!/usr/bin/env python3
"""Regression test: born2root's build dashboard, driven the way `make all`
drives it -- from an INTERACTIVE hellish, through make and /bin/sh, into a
hellish running the orchestrator.

born2root's `make all` (generate/orchestrate.sh) draws a live dashboard:
each step runs in the foreground while a background subshell repaints one
spinner glyph ten times a second with

    printf "\\0337\\033[%dA\\r\\033[5C\\033[1;34m%s\\033[0m\\0338" ...

-- DECSC, move up, draw, DECRC -- and is stopped with `kill $!; wait $!`
under `set -e`. Launched from hellish, born2root's Makefile picks hellish
for its scripts (the launcher probe: make's parent's comm), so the whole
chain is hellish at both ends. Two hellish bugs met there, and a user saw
them as one thing: garbage on the prompt and a build that died.

  1. printf read "\\0337" as echo -e's \\0nnn -- one byte, 0xDF -- where
     bash reads C's \\NNN: ESC then '7'. No cursor was ever saved, so the
     DECRC that followed each frame jumped to a stale position, glyphs
     landed on random rows, and a raw 0xDF sat on the prompt as '?'.
  2. `( ... ) &` forked twice, so $! named a middle process with no trap.
     `kill $!` killed it outright, `wait $!` said 143, `set -e` ended the
     orchestrator (make: Error 143), and the spinner body ran on as an
     orphan -- still repainting over the prompt that came back.

This drives a distilled copy of that orchestrator -- same traps, same
spinner, same kill/wait, same launcher probe in the Makefile -- and checks
what the user would see, for hellish and for bash alike:

  - the orchestrator really ran under the launching shell (the probe);
  - a clean run prints its last line and make exits 0;
  - not one 0xDF byte reached the terminal, and DECSC (ESC 7) did;
  - every spinner body is dead once make returns: nothing keeps drawing
    over the prompt, and no glyph appears after the shell's own output;
  - ^C during a step runs the INT trap ("Dashboard stopped"), shows the
    cursor again, kills the spinner, and leaves a shell that still answers.

Usage: python3 born2root_dashboard_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
BASH = os.path.expanduser("~/bash-5.3.9/bin/bash")
if not os.path.exists(BASH):
    BASH = "/bin/bash"
FAILS = []

GLYPH = "⠀".encode()[:2]      # every braille glyph starts e2 a0 / e2 a1
GLYPHS = (b"\xe2\xa0", b"\xe2\xa1")

ORCHESTRATE = r'''#!/bin/bash
# Distilled from born2root/generate/orchestrate.sh: the traps, the spinner
# subshell, the kill/wait under set -e, one quick step and one slow one.
set -e
STATE="$1"
readlink /proc/$$/exe > "$STATE/shell"
cleanup() {
	local sig="$1"
	stop_spinner 2> /dev/null || true
	printf '\033[?25h'
	if [ "$sig" = "INT" ] || [ "$sig" = "TERM" ]; then
		printf '\n  Dashboard stopped\n'
		exit 130
	fi
	printf '\ncleanup EXIT\n'
}
trap 'cleanup EXIT' EXIT
trap 'cleanup INT' INT
trap 'cleanup TERM' TERM
SPINNER_PID=""
start_spinner() {
	local lines_up="$1"
	(
		trap 'exit 0' TERM INT
		sh -c 'echo $PPID' >> "$STATE/bodies"
		local f=('⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠇' '⠏')
		local i=0
		while true; do
			printf "\0337\033[%dA\r\033[5C\033[1;34m%s\033[0m\0338" \
				"$lines_up" "${f[$i]}"
			i=$(((i + 1) % 10))
			sleep 0.1
		done
	) &
	SPINNER_PID=$!
	echo "$SPINNER_PID" >> "$STATE/spinners"
}
stop_spinner() {
	if [ -n "$SPINNER_PID" ]; then
		kill "$SPINNER_PID" 2> /dev/null
		wait "$SPINNER_PID" 2> /dev/null
		SPINNER_PID=""
	fi
}
run_step() {
	local name="$1"
	shift
	printf '  [ ] %s\n' "$name"
	start_spinner 1
	local rc=0
	"$@" > "$STATE/$name.log" 2>&1 || rc=$?
	stop_spinner
	if [ "$rc" -ne 0 ]; then
		printf '  step %s failed rc=%s\n' "$name" "$rc"
		exit 1
	fi
	printf '\033[1A\r  [x] %s\n' "$name"
}
printf '\033[?25l'
run_step check env NO_COLOR=1 "${SCRIPT_SH:-bash}" "$(dirname "$0")/check.sh"
run_step install sleep 2.5
printf 'ALL DONE\n'
'''

CHECK = r'''#!/usr/bin/env bash
# Stands in for setup/host/check_vbox_driver.sh: set -uo pipefail, a
# BASH_SOURCE-relative HERE, a command substitution, a grep pipeline.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
VER=$(printf '7.0.18r162988\n' | grep -oE '^[0-9]+\.[0-9]+\.[0-9]+r?[0-9]*' | tail -1)
[ -n "$VER" ] || exit 1
printf '  ok VirtualBox %s from %s\n' "$VER" "$HERE"
exit 0
'''

# born2root's launcher probe, verbatim: make's parent's comm, then $SHELL,
# then hellish, then bash -- the first that runs the scripts' bash-isms.
MAKEFILE = '''SCRIPT_SH := $(shell \\
	up=$$(ps -o ppid= -p $$PPID 2>/dev/null | tr -d " "); \\
	launcher=$$(ps -o comm= -p "$$up" 2>/dev/null | sed "s/^-//"); \\
	for c in "$$launcher" "$$SHELL" hellish bash; do \\
		[ -n "$$c" ] || continue; \\
		p=$$(command -v "$$c" 2>/dev/null) || continue; \\
		"$$p" -c 'a=(1 2); f(){ local x=0; }; f; : "$${BASH_SOURCE[0]:-x}"' >/dev/null 2>&1 \\
			&& { printf "%s" "$$p"; break; }; \\
	done)
SCRIPT_SH := $(if $(strip $(SCRIPT_SH)),$(strip $(SCRIPT_SH)),bash)
export SCRIPT_SH
all:
\t@$(SCRIPT_SH) $(CURDIR)/orchestrate.sh $(STATE)
'''


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def make_fixture():
    """The Makefile, the orchestrator and its step script, plus a bin dir
    where `hellish` is the shell under test -- the probe resolves the
    launcher's comm through PATH, and it must find THIS build, not
    whatever hellish is installed on the machine."""
    fix = tempfile.mkdtemp(prefix="hellish_b2r_fix_")
    for name, body in (("orchestrate.sh", ORCHESTRATE), ("check.sh", CHECK),
                       ("Makefile", MAKEFILE)):
        with open(os.path.join(fix, name), "w") as f:
            f.write(body)
        os.chmod(os.path.join(fix, name), 0o755)
    bindir = os.path.join(fix, "bin")
    os.mkdir(bindir)
    os.symlink(SHELL, os.path.join(bindir, "hellish"))
    return fix, bindir


def pty_run(argv, env, seq):
    """Run argv on a pty, feed each (bytes, settle) pair, return raw bytes."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(argv[0], argv)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    out = b""
    time.sleep(0.7)
    for data, wait in seq:
        os.write(fd, data)
        end = time.time() + wait
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return out


def read_pids(path):
    try:
        with open(path) as f:
            return [int(x) for x in f.read().split()]
    except (OSError, ValueError):
        return []


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def read_text(path):
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return ""


def drive(shell, fix, bindir, interrupt):
    state = tempfile.mkdtemp(prefix="hellish_b2r_state_")
    env = {
        "HOME": os.environ.get("HOME", "/tmp"),
        "PATH": bindir + ":" + os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    cmd = ("make -s -C %s STATE=%s all; echo RC=$?\n" % (fix, state)).encode()
    if interrupt:
        seq = [(cmd, 1.8), (b"\x03", 2.5), (b"echo PROMPT_OK\n", 1.5)]
    else:
        seq = [(cmd, 7.0), (b"echo PROMPT_OK\n", 1.5)]
    out = pty_run([shell, "--norc"], env, seq)
    time.sleep(0.3)
    bodies = read_pids(os.path.join(state, "bodies"))
    spinners = read_pids(os.path.join(state, "spinners"))
    ran_under = read_text(os.path.join(state, "shell"))
    orphans = [p for p in bodies + spinners if alive(p)]
    for p in orphans:
        try:
            os.kill(p, 9)
        except OSError:
            pass
    shutil.rmtree(state, ignore_errors=True)
    return out, ran_under, bodies, orphans


def glyph_after(out, marker):
    tail = out.split(marker)[-1] if marker in out else out
    return any(g in tail for g in GLYPHS)


def run_cases(label, shell, expect_exe):
    fix, bindir = make_fixture()
    try:
        out, ran_under, bodies, orphans = drive(shell, fix, bindir, False)
        tag = label + ": clean run: "
        check(tag + "orchestrator ran under the launching shell",
              expect_exe(ran_under), "ran under %r" % ran_under)
        check(tag + "both steps started a spinner body", len(bodies) == 2,
              "bodies=%r" % bodies)
        check(tag + "make finished and reported success",
              b"ALL DONE" in out and b"RC=0" in out, repr(out[-400:]))
        check(tag + "EXIT trap ran once the steps were done",
              b"cleanup EXIT" in out)
        check(tag + "no 0xDF byte reached the terminal", b"\xdf" not in out)
        check(tag + "the spinner saved the cursor with ESC 7", b"\x1b7" in out)
        check(tag + "no spinner body outlived make", not orphans,
              "orphans=%r" % orphans)
        check(tag + "nothing drew over the prompt after make returned",
              not glyph_after(out, b"RC=0"), repr(out.split(b"RC=0")[-1][:200]))
        check(tag + "the shell still answers", b"PROMPT_OK\r\n" in out)

        out, ran_under, bodies, orphans = drive(shell, fix, bindir, True)
        tag = label + ": ^C mid-step: "
        check(tag + "the INT trap ran", b"Dashboard stopped" in out,
              repr(out[-400:]))
        check(tag + "the run did not carry on past the ^C",
              b"ALL DONE" not in out)
        check(tag + "the cursor was shown again after the trap",
              b"Dashboard stopped" in out
              and b"\x1b[?25h" in out.split(b"Dashboard stopped")[-1])
        check(tag + "no spinner body outlived the ^C", not orphans,
              "orphans=%r" % orphans)
        check(tag + "the shell still answers", b"PROMPT_OK\r\n" in out)
        check(tag + "nothing drew over the prompt afterwards",
              not glyph_after(out, b"PROMPT_OK\r\n"))
    finally:
        shutil.rmtree(fix, ignore_errors=True)


def main():
    if not shutil.which("make") or not shutil.which("ps"):
        print("SKIP: needs make and ps on PATH")
        return 0
    if not os.access(SHELL, os.X_OK):
        print("error: %s is not executable" % SHELL)
        return 2
    real = os.path.realpath(SHELL)
    run_cases("hellish", SHELL, lambda p: os.path.realpath(p) == real)
    # The same fixture under bash proves the harness itself: every check
    # above is a bash behaviour, not a hellish invention.
    run_cases("bash", BASH, lambda p: os.path.basename(p).startswith("bash"))
    print()
    if FAILS:
        print("FAILED: %d" % len(FAILS))
        for f in FAILS:
            print("  - " + f)
        return 1
    print("ALL PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())

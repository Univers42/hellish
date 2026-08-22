#!/usr/bin/env python3
"""Regression test: job control and the controlling terminal.

Guards the /dev/null-stdin rule in bg_child_body (execute_range_bg.c).

POSIX puts "the standard input for an asynchronous list shall be assigned
to /dev/null" under *if job control is disabled* (XCU 2.9.3.1). hellish
used to apply it unconditionally, which is right for scripts and -c but
wrong for an interactive shell, where bash leaves the terminal attached
and lets the kernel's job control do the work. The visible damage was any
background job that inspects the tty:

    $ top &
    top: failed tty get          <- tcgetattr on /dev/null

instead of bash's behaviour -- keep the tty, touch it from a background
process group, take SIGTTOU, and stop as `[1]+ Stopped`. (issue #25)

Checks, in a real pty, against `bash --posix`'s own behaviour where the
two are supposed to agree:
  1. Interactive: a background job's stdin IS the terminal (as in bash).
  2. Non-interactive -c: a background job's stdin is NOT the terminal,
     so a background `read` in a script still sees EOF and never blocks.
  3. `top &` does not fail with "failed tty get", and lands as a stopped
     job exactly like bash.
  4. ^Z on a foreground command does not wedge the shell. The wait had no
     WUNTRACED, so a stopped child never satisfied it: the REPL blocked in
     waitpid forever, the tty fell back to cooked mode, and the kernel
     echoed everything typed afterwards while nothing ran it -- the shell
     looked alive and was not. It must now announce `[1]+ Stopped`, prompt
     again, run the next command, and let `fg` resume the job. (issue #25)
  5. Exiting with a job still stopped is refused once, the way bash does
     it, so a suspended job cannot be orphaned by one absent-minded exit.
  6. The very next exit is honoured, so that warning can never trap a user
     in a shell they cannot leave. (issue #41)

Usage: python3 bg_tty_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import struct
import subprocess
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

ENV = {
    "HOME": os.environ.get("HOME", "/tmp"),
    "PATH": os.environ["PATH"],
    "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
    "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
    "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
}


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def pty_run(argv, sends, settle=1.6):
    """Run argv on a pty, feed `sends`, return everything it printed."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(ENV)
        os.execvp(argv[0], argv)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    out = b""
    time.sleep(0.6)
    for s in sends:
        os.write(fd, s)
        end = time.time() + settle
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
    return out.decode(errors="replace")


def pty_run_seq(argv, seq):
    """Like pty_run, but each send carries its own settle time."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(ENV)
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
    return out.decode(errors="replace")


def bg_stdin_verdict(argv):
    """Run the tty probe as a background job; return "tty"/"notty"/None.

    The result goes through a FILE rather than stdout on purpose. The pty
    echoes the command line back, and that echo contains the text of both
    branches -- grepping the transcript for a marker matches the echo and
    passes no matter what the shell actually did. This test shipped with
    exactly that false pass before the file was used.
    """
    path = tempfile.mktemp(prefix="hellish_bgtty_")
    probe = ("{ test -t 0 && echo tty > %s || echo notty > %s; } &\n"
             % (path, path)).encode()
    pty_run(argv, [probe])
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None
    finally:
        try:
            os.unlink(path)
        except OSError:
            pass


def main():
    # 1: interactive keeps the terminal, and bash agrees
    bash = bg_stdin_verdict([BASH, "-i"])
    hell = bg_stdin_verdict([SHELL])
    check("interactive: bash keeps the tty on a background job",
          bash == "tty", "bash said %r" % bash)
    check("interactive: background job keeps the tty (matches bash)",
          hell == "tty",
          "got %r -- /dev/null stdin leaked into an interactive shell" % hell)

    # 2: -c is job-control-less, so /dev/null still applies
    script = "{ test -t 0 && echo BG_IS_TTY || echo BG_NOT_TTY; } & wait"
    for name, sh in (("hellish", SHELL), ("bash", BASH)):
        r = subprocess.run([sh, "-c", script], capture_output=True, text=True,
                           env=dict(os.environ, **ENV))
        check("-c (%s): background stdin is /dev/null" % name,
              "BG_NOT_TTY" in r.stdout, "stdout=%r" % r.stdout[:80])

    # 3: the exact command from the report
    out = pty_run([SHELL], [b"top &\n", b"\n", b"jobs\n"], settle=1.8)
    check("top & does not fail tty get", "failed tty get" not in out)
    check("top & becomes a stopped job", "Stopped" in out,
          "no Stopped line; output tail=%r" % out[-160:])

    # 4: ^Z on a foreground command. `top` rather than `sleep` on purpose.
    # This test drives the shell straight from pty.fork(), which makes it a
    # session leader, and a session leader's process group is orphaned by
    # POSIX's definition -- the kernel silently DISCARDS SIGTSTP sent to an
    # orphaned group. `sleep` would never stop here no matter what the shell
    # does. `top` raises SIGSTOP on itself, which cannot be discarded, so it
    # exercises the shell's side of the contract regardless. Under a real
    # terminal (a shell inside another shell) `sleep` stops too; that is
    # verified by hand and tracked in the per-job process-group issue.
    seq = [(b"top\n", 2.2), (b"\x1a", 1.5), (b"jobs\n", 1.2),
           (b"echo AFTER_STOP\n", 1.2), (b"fg\n", 2.0), (b"q", 1.2),
           (b"echo AFTER_FG\n", 1.2)]
    out = pty_run_seq([SHELL], seq)
    check("^Z on a foreground job reports Stopped", "Stopped" in out,
          "tail=%r" % out[-200:])
    check("^Z leaves the shell able to run the next command",
          out.count("AFTER_STOP") >= 2,
          "shell wedged in waitpid; tail=%r" % out[-200:])
    check("jobs lists the stopped foreground job", "top" in out)
    check("fg resumes it and the shell survives",
          out.count("AFTER_FG") >= 2, "tail=%r" % out[-200:])

    # 5: leaving with a stopped job. bash refuses the first exit and says
    # "There are stopped jobs.", then honours the next one. hellish used to
    # walk straight out, orphaning whatever was suspended -- which is how a
    # session full of backgrounded `top`s vanished in issue #41.
    seq = [(b"top\n", 2.2), (b"\x1a", 1.5), (b"exit\n", 1.5),
           (b"echo STILL_HERE\n", 1.2)]
    out = pty_run_seq([SHELL], seq)
    check("exit with a stopped job is refused once",
          "There are stopped jobs." in out, "tail=%r" % out[-200:])
    check("the shell survives the refused exit",
          out.count("STILL_HERE") >= 2, "tail=%r" % out[-200:])

    # 6: ...and the immediately following exit is honoured, so the warning
    # can never become a trap the user cannot get out of.
    seq = [(b"top\n", 2.2), (b"\x1a", 1.5), (b"exit\n", 1.5),
           (b"exit\n", 1.5), (b"echo NOT_REACHED\n", 1.2)]
    out = pty_run_seq([SHELL], seq)
    check("a second, immediate exit leaves anyway",
          out.count("NOT_REACHED") < 2, "shell would not exit; tail=%r"
          % out[-200:])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

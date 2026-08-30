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
    """Run argv on a pty, feed `sends`, return everything it printed.

    Every caller passes --norc (bash and hellish both take it). An
    interactive shell that reads the developer's rc is not running the
    configuration this test describes, and the ways that goes wrong are
    quiet ones -- see the header of prompt_jobs_badge_test.py.
    """
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


def _pgrp_tpgid(path):
    """(pgrp, tpgid) out of a /proc/<pid>/stat dump.

    Split after the LAST ')' -- the comm field is parenthesised and may
    itself contain spaces, so a plain .split() mis-aligns every field.
    Past comm the order is state, ppid, pgrp, session, tty_nr, tpgid.
    """
    with open(path) as f:
        s = f.read()
    fields = s[s.rindex(")") + 1:].split()
    return int(fields[2]), int(fields[5])


def fg_pgrps(argv):
    """Run a foreground command that reports its own process group.

    Returns (child_pgrp, child_tpgid, shell_pgrp).  Both dumps go through
    FILES for the reason documented on bg_stdin_verdict: the pty echoes
    the command line back, so anything grepped out of the transcript can
    match the echo instead of the result.
    """
    fc = tempfile.mktemp(prefix="hellish_fgpg_")
    fs = tempfile.mktemp(prefix="hellish_shpg_")
    pty_run(argv, [("cat /proc/self/stat > %s\n" % fc).encode(),
                   ("cat /proc/$$/stat > %s\n" % fs).encode()])
    try:
        child_pg, child_tpgid = _pgrp_tpgid(fc)
        shell_pg, _ = _pgrp_tpgid(fs)
        return child_pg, child_tpgid, shell_pg
    except (OSError, ValueError, IndexError):
        return None, None, None
    finally:
        for p in (fc, fs):
            try:
                os.unlink(p)
            except OSError:
                pass


def main():
    # 1: interactive keeps the terminal, and bash agrees
    # --norc goes BEFORE -i on the bash side: bash refuses
    # `bash -i --norc` outright ("--: invalid option").
    bash = bg_stdin_verdict([BASH, "--norc", "-i"])
    hell = bg_stdin_verdict([SHELL, "--norc"])
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
    out = pty_run([SHELL, "--norc"],
                  [b"top &\n", b"\n", b"jobs\n"], settle=1.8)
    check("top & does not fail tty get", "failed tty get" not in out)
    check("top & becomes a stopped job", "Stopped" in out,
          "no Stopped line; output tail=%r" % out[-160:])

    # 4: ^Z on a foreground command, driven through `top`.
    # `top` raises SIGSTOP on itself, which no rule can discard, so this
    # check exercises the shell's side of the contract (WUNTRACED, the
    # Stopped notice, `fg`) independently of process groups. Check 8 below
    # covers the same ground with a command that does NOT self-stop, which
    # is the part that needs per-job process groups to work at all.
    seq = [(b"top\n", 2.2), (b"\x1a", 1.5), (b"jobs\n", 1.2),
           (b"echo AFTER_STOP\n", 1.2), (b"fg\n", 2.0), (b"q", 1.2),
           (b"echo AFTER_FG\n", 1.2)]
    out = pty_run_seq([SHELL, "--norc"], seq)
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
    out = pty_run_seq([SHELL, "--norc"], seq)
    check("exit with a stopped job is refused once",
          "There are stopped jobs." in out, "tail=%r" % out[-200:])
    check("the shell survives the refused exit",
          out.count("STILL_HERE") >= 2, "tail=%r" % out[-200:])

    # 6: ...and the immediately following exit is honoured, so the warning
    # can never become a trap the user cannot get out of.
    seq = [(b"top\n", 2.2), (b"\x1a", 1.5), (b"exit\n", 1.5),
           (b"exit\n", 1.5), (b"echo NOT_REACHED\n", 1.2)]
    out = pty_run_seq([SHELL, "--norc"], seq)
    check("a second, immediate exit leaves anyway",
          out.count("NOT_REACHED") < 2, "shell would not exit; tail=%r"
          % out[-200:])

    # 7: a foreground command must run in a process group of its OWN, and
    # the terminal must be handed to that group. Without this the shell's
    # group is orphaned by POSIX's definition (no member has a parent in a
    # different group of the same session, once the shell is a session
    # leader), and the kernel DISCARDS every SIGTSTP/SIGTTIN/SIGTTOU sent
    # to it -- so ^Z did not merely aim badly, it did nothing at all and
    # the command kept eating the keystrokes meant for the shell. (#27)
    for name, argv in (("bash", [BASH, "--norc", "-i"]),
                       ("hellish", [SHELL, "--norc"])):
        cpg, ctpgid, spg = fg_pgrps(argv)
        check("%s: foreground command gets its own process group" % name,
              cpg is not None and spg is not None and cpg != spg,
              "child pgrp=%s shell pgrp=%s" % (cpg, spg))
        check("%s: the terminal is handed to the foreground job" % name,
              cpg is not None and cpg == ctpgid,
              "child pgrp=%s tpgid=%s" % (cpg, ctpgid))

    # 8: the case check 4 cannot reach. `cat` has no self-STOP to fall back
    # on, so it stops only if the signal is really delivered -- which is
    # true only once the job has a process group of its own.
    seq = [(b"cat\n", 1.2), (b"\x1a", 1.5), (b"echo AFTER_TSTP\n", 1.3),
           (b"jobs\n", 1.3)]
    out = pty_run_seq([SHELL, "--norc"], seq)
    check("^Z stops a command that does not stop itself",
          "Stopped" in out, "tail=%r" % out[-200:])
    check("the shell runs the next command after that ^Z",
          out.count("AFTER_TSTP") >= 2, "tail=%r" % out[-200:])

    # 9: `kill %1` on a STOPPED job. The signal stays pending until
    # something resumes the process, so this looked like a no-op: the job
    # sat there as "Stopped" for the rest of the session. bash follows the
    # signal with SIGCONT; and job_update_status has to keep polling a
    # stopped job or the shell can never notice it died.
    seq = [(b"cat\n", 1.2), (b"\x1a", 1.5), (b"kill %1\n", 1.5),
           (b"echo AFTER_KILL\n", 1.3), (b"jobs\n", 1.5)]
    out = pty_run_seq([SHELL, "--norc"], seq)
    check("kill %1 terminates a stopped job", "Terminated" in out,
          "tail=%r" % out[-240:])
    check("the killed job then leaves the jobs table",
          "Stopped" not in out.rsplit("AFTER_KILL", 1)[-1],
          "still listed; tail=%r" % out[-240:])

    # 10: none of this may leak into a non-interactive shell. Scripts and
    # -c have job control disabled, so a foreground command shares the
    # shell's group there -- in bash too. This is the guard that keeps the
    # fix off the hot path the benchmarks measure.
    probe = "cat /proc/self/stat; cat /proc/$$/stat"
    for name, sh in (("hellish", SHELL), ("bash", BASH)):
        r = subprocess.run([sh, "-c", probe], capture_output=True, text=True,
                           env=dict(os.environ, **ENV))
        lines = [l for l in r.stdout.splitlines() if ")" in l]
        pgs = [int(l[l.rindex(")") + 1:].split()[2]) for l in lines]
        check("-c (%s): no process groups are created" % name,
              len(pgs) == 2 and pgs[0] == pgs[1], "pgrps=%s" % pgs)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

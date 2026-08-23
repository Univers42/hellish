#!/usr/bin/env python3
"""Regression test: leaving a shell that still holds stopped jobs -- issue #58.

Reported with `top &` followed by Ctrl-D: hellish walked out, and the
terminal it left behind was unusable -- no echo, mangled input, commands
like `bashecho` appearing out of nowhere. `top` had put the tty in raw mode
before the kernel stopped it, and with the shell gone nobody put it back.

bash never gets there. It refuses the first exit while a stopped job
exists, says "There are stopped jobs.", and only leaves if you ask twice.
hellish HAS that guard (exit_stopped_guard, builtins/exit_jobs.c) and it
did not work, for two independent reasons:

  1. Ctrl-D never consulted it. handle_eof_or_error() set should_exit
     directly, so the guard only ever protected the `exit` BUILTIN. Every
     report of this bug came in through Ctrl-D.

  2. It read a stale job table. A job stopped by SIGTTIN/SIGTTOU is only
     noticed when the shell next reaps -- at the top of a later REPL turn.
     `cat &` immediately followed by `exit` therefore saw JOB_RUNNING and
     let the user out, while the same pair with any command in between saw
     JOB_STOPPED and warned. That is the "sometimes it works, sometimes it
     doesn't" in the report: a race, not a flake.

The choice this pins, since bash and other shells differ: hellish follows
bash on the WARNING -- the first exit is refused, the second is obeyed --
and then goes one step further on the way out, sending SIGCONT+SIGHUP to
what is left. bash can afford to leave a stopped job behind because it is
usually the session leader and the kernel hangs the job up when the
terminal goes. A nested hellish is not the session leader, so a job left
stopped there lingers forever holding a raw terminal, which is precisely
the damage being fixed.

Every logic case below is also run against bash, which is the oracle for
the warning behaviour.

Usage: python3 exit_stopped_jobs_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import struct
import subprocess
import sys
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "build/bin/hellish")
BASH = "/bin/bash"
FAILS = []
ESC = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def drive(sh, cmds, settle=1.5):
    """Run cmds on a pty. Returns (screen, still_alive, tty_is_sane, pids)."""
    interactive = ["-i"] if sh == BASH else []
    env = {"HOME": os.environ.get("HOME", "/tmp"), "PATH": os.environ["PATH"],
           "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "P> ",
           "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0"}
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execv(sh, [sh] + interactive)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    time.sleep(0.9)
    out = b""
    for c in cmds:
        os.write(fd, c)
        end = time.time() + settle
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
    time.sleep(0.6)
    try:
        wpid, _ = os.waitpid(pid, os.WNOHANG)
        alive = (wpid != pid)
    except OSError:
        alive = False
    # ICANON+ECHO is what a shell prompt needs; a raw-mode child left behind
    # clears both, and that is exactly the terminal the report was left in.
    try:
        lflag = termios.tcgetattr(fd)[3]
        sane = bool(lflag & termios.ICANON) and bool(lflag & termios.ECHO)
    except Exception:
        sane = False
    if alive:
        # killpg, not kill: pty.fork() made this child a session leader, and
        # SIGKILL means it never runs its own exit path -- so its background
        # jobs would be orphaned and counted against the NEXT case.
        try:
            os.killpg(pid, 9)
        except OSError:
            pass
        try:
            os.kill(pid, 9)
            os.waitpid(pid, 0)
        except OSError:
            pass
    os.close(fd)
    return ESC.sub("", out.decode(errors="replace")).replace("\r\n", "\n"), \
        alive, sane


WARN = "There are stopped jobs."


def both(label, cmds, want_alive, want_warn, settle=1.5):
    """Assert hellish matches bash, and state what bash actually did."""
    bt, ba, _ = drive(BASH, cmds, settle)
    ht, ha, hs = drive(SHELL, cmds, settle)
    bash_says = (ba, WARN in bt)
    check("%s: bash is the oracle (alive=%s warn=%s)" % (label, want_alive,
                                                         want_warn),
          bash_says == (want_alive, want_warn),
          "bash gave alive=%s warn=%s" % bash_says)
    check("%s: warns" % label, (WARN in ht) == want_warn,
          "screen: %r" % ht[-260:])
    check("%s: %s" % (label, "stays" if want_alive else "leaves"),
          ha == want_alive, "screen: %r" % ht[-260:])
    return ht, ha, hs


def main():
    # 1. The race. `cat &` gets SIGTTIN the instant it reads the tty; an
    #    exit typed immediately after must still see it.
    both("cat& then exit at once", [b"cat &\n", b"exit\n"], True, True)

    # 2. The path every report came in through.
    both("cat& then ^D at once", [b"cat &\n", b"\x04"], True, True)

    # 3. With a command in between -- the case that already worked, kept so
    #    the fix cannot regress it.
    both("cat& , echo, then exit", [b"cat &\n", b"echo x\n", b"exit\n"],
         True, True)
    # Deliberately NOT bash parity. bash's `jobs` marks what it lists as
    # notified and then leaves without a word -- run `jobs`, press Ctrl-D,
    # and bash silently abandons your stopped jobs. hellish warns anyway:
    # it is usually a NESTED shell, not the session leader, so nothing will
    # hang the job up for it and the terminal stays hostage.
    txt, alive, _ = drive(SHELL, [b"cat &\n", b"jobs\n", b"\x04"])
    check("cat& , jobs, then ^D: still warns (stricter than bash)",
          WARN in txt and alive, "screen: %r" % txt[-260:])

    # 4. Asking twice gets you out, and the terminal comes back usable.
    _, alive, sane = drive(SHELL, [b"cat &\n", b"exit\n", b"exit\n"])
    check("a second exit is obeyed", not alive, "shell would not leave")
    check("the terminal is usable afterwards", sane,
          "left without ICANON/ECHO")
    _, alive, sane = drive(SHELL, [b"cat &\n", b"\x04", b"\x04"])
    check("a second ^D is obeyed", not alive)
    check("the terminal is usable after ^D too", sane)

    # 5. The reported shape: a background job that puts the tty in RAW mode
    #    before it is stopped. This is the one that ruins the terminal.
    raw = b"sh -c 'stty raw -echo; sleep 30' &\n"
    txt, alive, sane = drive(SHELL, [raw, b"\x04"], settle=2.0)
    check("a raw-mode background job does not let ^D through",
          alive and WARN in txt, "screen: %r" % txt[-300:])
    txt, alive, sane = drive(SHELL, [raw, b"\x04", b"\x04"], settle=2.0)
    check("after a deliberate second ^D the terminal is restored", sane,
          "terminal left raw -- this is the reported damage")

    # 6. Nothing left frozen on the tty once we do leave. A stopped job that
    #    outlives a nested shell holds the terminal forever.
    txt, alive, _ = drive(SHELL, [b"cat &\n", b"exit\n", b"exit\n"])
    time.sleep(0.4)
    leftover = subprocess.run(
        ["pgrep", "-f", "^cat$"], capture_output=True, text=True).stdout.split()
    check("no stopped job is left behind holding the terminal",
          not leftover, "still running: %r" % leftover)

    # 6b. The warning RE-ARMS after any command -- the shape from the second
    #     report. Warn once, run something, press Ctrl-D again: the shell must
    #     warn again, not walk out over a job it merely mentioned earlier.
    #     exit_warned used to be cleared only by a BUILTIN, so an external
    #     command in between left it set and the next Ctrl-D left.
    for label, between in (("an external command", b"ps\n"),
                           ("a builtin", b"jobs\n")):
        txt, alive, _ = drive(SHELL, [b"cat &\n", b"\x04", between, b"\x04"],
                              settle=1.5)
        check("after %s, the next ^D warns again" % label,
              txt.count(WARN) >= 2 and alive,
              "warnings=%d alive=%s: %r" % (txt.count(WARN), alive,
                                            txt[-300:]))

    # 6c. Many stopped jobs at once, the exact shape of the report: seven
    #     `top &`. One Ctrl-D must still be refused -- the count is not what
    #     the guard keys off, but a stale warning flag made it look like it.
    many = [b"cat &\n"] * 5 + [b"\x04"]
    txt, alive, _ = drive(SHELL, many, settle=0.9)
    check("five stopped jobs: one ^D is still refused",
          WARN in txt and alive, "alive=%s: %r" % (alive, txt[-300:]))

    # 6d. And when you DO leave, the terminal comes back -- even though the
    #     jobs that were holding it were stopped in raw mode. This is the
    #     damage in the report: no echo, keystrokes spliced into garbage.
    raw = b"sh -c 'stty raw -echo; sleep 30' &\n"
    _, alive, sane = drive(SHELL, [raw, raw, b"\x04", b"\x04"], settle=1.6)
    check("two raw-mode jobs: the terminal is restored on the way out",
          (not alive) and sane, "alive=%s sane=%s" % (alive, sane))

    # 6e. MANY raw-mode jobs, which is the shape that still broke after the
    #     guard was fixed. Hanging up N full-screen programs means N of them
    #     repaint, restore their own terminal idea and print a farewell on
    #     the way out. If the shell restores the tty and exits first, all of
    #     that lands afterwards, on a terminal that now belongs to the parent
    #     -- a screenful of blank lines and the next command spliced into the
    #     debris. The shell must drain them BEFORE handing the terminal back.
    many_raw = [b"sh -c 'stty raw -echo; sleep 30' &\n"] * 8
    txt, alive, sane = drive(SHELL, many_raw + [b"\x04", b"\x04"], settle=0.8)
    check("eight raw-mode jobs: the shell does leave", not alive,
          "still alive after two ^D")
    check("eight raw-mode jobs: the terminal is left usable", sane,
          "terminal handed back without ICANON/ECHO")

    # 6f. And nothing of ours is still running once we are gone: a job left
    #     behind by a NESTED shell has no session leader to hang it up, so it
    #     would sit there stopped, holding the terminal, forever. Measured
    #     against a marker unique to this case so an earlier one cannot bleed
    #     into it.
    marker = "hellish_hangup_probe_%d" % os.getpid()
    probe = ("sh -c 'stty raw -echo; sleep 30 %s' &\n" % marker).encode()
    _, alive, sane = drive(SHELL, [probe] * 4 + [b"\x04", b"\x04"],
                           settle=0.8)
    check("marked raw-mode jobs: the shell leaves cleanly",
          (not alive) and sane, "alive=%s sane=%s" % (alive, sane))
    time.sleep(0.6)
    left = subprocess.run(["pgrep", "-f", marker],
                          capture_output=True, text=True).stdout.split()
    check("marked raw-mode jobs: none survive the shell", not left,
          "left running: %r" % left)

    # 7. No false positives: a shell with no stopped job just leaves.
    _, alive, _ = drive(SHELL, [b"echo hi\n", b"exit\n"])
    check("no stopped jobs: exit leaves at once", not alive)
    _, alive, _ = drive(SHELL, [b"echo hi\n", b"\x04"])
    check("no stopped jobs: ^D leaves at once", not alive)

    # 8. Non-interactive means it. A script that says exit, exits.
    p = subprocess.run([SHELL, "-c", "cat & exit 0"], capture_output=True,
                       text=True, timeout=20, stdin=subprocess.DEVNULL)
    check("a script's exit is never second-guessed",
          p.returncode == 0 and WARN not in p.stderr,
          "rc=%d err=%r" % (p.returncode, p.stderr[:160]))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

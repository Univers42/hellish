#!/usr/bin/env python3
"""#85: a foreground program killed mid-password leaves the terminal
with echo off, and the shell has to take it back.

    $ chsh
    Password: ^C
    $ <nothing you type appears>

chsh, ssh and sudo all turn echo off to read a password and turn it back
on when they finish.  Interrupt one and it never reaches the second half,
so the terminal keeps the setting -- and the shell is the only thing still
running that can undo it.  The reporter's words: "the TTY is disactivated
and crash, we can no longer see the input written".

The interesting half of this test is the case that must NOT be fixed.
`stty -echo` typed as a command is a deliberate request, and a shell that
restored the terminal after every command would reverse it.  bash draws
the line at "was the job killed by a signal", and so does this:

    stty -echo                       echo stays off       both shells
    <program that disables echo> ^C  echo comes back      bash, and now us
    sleep 30 ^C                      unaffected           control

The control matters: it separates "the shell mishandles Ctrl-C" from "the
shell does not reclaim the terminal", which look identical from the
outside and have nothing to do with each other.  Only the second was true.
"""
import os
import pty
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SHELL = os.environ.get("HELLISH_BIN", os.path.join(ROOT, "build/bin/hellish"))
FAILS = []

# A stand-in for chsh: turn echo off, print a prompt, then block. It is
# spelled out here rather than calling the real chsh because chsh wants
# root, a real account, and a password -- none of which belong in a test.
NOECHO = r"""
import termios, sys, time
fd = sys.stdin.fileno()
a = termios.tcgetattr(fd)
a[3] &= ~termios.ECHO
termios.tcsetattr(fd, termios.TCSANOW, a)
sys.stderr.write("Password: ")
sys.stderr.flush()
time.sleep(30)
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def drain(fd, seconds=1.2):
    time.sleep(seconds)
    os.set_blocking(fd, False)
    try:
        return os.read(fd, 65536).decode("utf8", "replace")
    except OSError:
        return ""
    finally:
        os.set_blocking(fd, True)


def echoes_after(cmd, interrupt):
    """Run cmd in an interactive shell on a pty, optionally Ctrl-C it, then
    type a command and report whether the TYPED TEXT came back.

    Reading the ECHO bit is not the test: readline turns ECHO off itself
    and redraws the line, so the bit is off in a healthy shell too. What
    the user notices, and what this asserts, is whether the characters
    appear."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.update(HELLISH_BANNER="0", HELLISH_NO_ANIM="1",
                          HELLISH_NO_UPDATE_CHECK="1", HOME=os.environ["HOME"])
        os.execv(SHELL, [SHELL, "-i"])
    try:
        drain(fd, 1.2)
        os.write(fd, cmd.encode() + b"\n")
        drain(fd, 1.4)
        if interrupt:
            os.write(fd, b"\x03")
            drain(fd, 1.2)
        os.write(fd, b"echo TYPED_BACK\n")
        tail = drain(fd, 1.2)
        return "echo TYPED_BACK" in tail, tail
    finally:
        try:
            os.kill(pid, 9)
            os.waitpid(pid, 0)
        except OSError:
            pass
        os.close(fd)


def main():
    if not os.path.exists(SHELL):
        print("no shell at %s" % SHELL)
        return 1
    helper = os.path.join(HERE, "test_files", "_noecho_helper.py")
    os.makedirs(os.path.dirname(helper), exist_ok=True)
    with open(helper, "w") as fh:
        fh.write(NOECHO)

    # The control: Ctrl-C alone must not cost the echo. If this fails the
    # rest of the file is measuring signal handling, not the terminal.
    seen, out = echoes_after("sleep 30", True)
    check("control: Ctrl-C on a plain command leaves typing visible", seen,
          "tail=%r" % out[-200:])

    # The bug.
    seen, out = echoes_after("%s %s" % (sys.executable, helper), True)
    check("a password prompt killed by Ctrl-C gives the terminal back", seen,
          "typing is invisible after the interrupt; tail=%r" % out[-200:])

    # The half that must stay broken-looking, because the user asked for it.
    seen, out = echoes_after("stty -echo", False)
    check("a deliberate `stty -echo` is NOT reversed", not seen,
          "the shell undid something the user asked for")

    # And the shell is still usable afterwards.
    seen, out = echoes_after("%s %s" % (sys.executable, helper), True)
    check("the shell still runs commands after all that",
          "TYPED_BACK" in out, "tail=%r" % out[-200:])

    try:
        os.unlink(helper)
    except OSError:
        pass
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

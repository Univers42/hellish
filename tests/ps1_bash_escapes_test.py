#!/usr/bin/env python3
"""#72 phase 3.7: the bash PS1 escapes that rendered as their own source.

`ps1_render` implemented most of bash's escape set and a handful of hellish
extensions on top. The ones it did not implement fell through to "emit
backslash + char literally", which is the right answer for an escape bash
does not know either -- and the wrong one for eight that bash does:

    \\D{fmt}  strftime, the only escape that takes an argument
    \\T       the clock, 12-hour
    \\@       the clock, 12-hour with am/pm
    \\!       history number
    \\#       command number
    \\l       the terminal's basename
    \\r       carriage return
    \\V       version

A PS1 copied out of someone's .bashrc therefore came out with `\\!` sitting
in it as two visible characters. Not a crash, not an error -- just a prompt
that reads like the shell is broken, which is the report this arrived as.

\\A is the one deliberate collision and it is NOT fixed here: bash's \\A is
the 24-hour clock, hellish's is the animation frame, and hellish's shipped
first. Changing it would break every prompt already using it, so the test
pins the collision instead of hiding it.

Everything checkable against the oracle is checked against it rather than
against a hand-written expectation -- \\D{} and \\T especially, whose output
depends on the locale the test happens to run in.

Usage: python3 ps1_bash_escapes_test.py [/path/to/hellish]
"""
import os
import pty
import re
import select
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def render(sh, ps1, extra=None):
    """Set PS1 in a real interactive shell and return what it painted.

    The prompt is bracketed with |...| so the assertions can cut it out of
    the surrounding terminal noise -- readline redraws, and a substring
    search over the whole transcript would match the PS1 assignment being
    echoed back rather than the rendered prompt.
    """
    # LC_ALL=C so strftime is the same on both shells and on every runner:
    # \@ is %p, and a locale where that is empty would turn a real
    # comparison into a shape check nobody notices has stopped comparing.
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_BANNER="0",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1", TERM="dumb",
               LC_ALL="C")
    env.pop("PS1", None)
    args = ["--norc", "-i"]
    if os.path.basename(sh).startswith("bash"):
        args = ["--norc", "-i"]
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(sh, [sh] + args, env)
        os._exit(1)
    out = b""

    def pump(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.15)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
    pump(1.0)
    os.write(fd, ("PS1='|" + ps1 + "|'\n").encode())
    pump(0.7)
    out = b""
    for c in (extra or ["true"]):
        os.write(fd, c.encode() + b"\n")
        pump(0.6)
    os.write(fd, b"exit\n")
    pump(0.3)
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except OSError:
        pass
    return out.decode("utf-8", "replace")


def fields(out):
    """Every |...| the shell painted, in order."""
    return re.findall(r"\|([^|\r\n]*)\|", out)


def both(ps1, extra=None):
    got = fields(render(SHELL, ps1, extra))
    ref = []
    if os.path.isfile(ORACLE):
        ref = fields(render(ORACLE, ps1, extra))
    return got, ref


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    if not os.path.isfile(ORACLE):
        print("note: no oracle at %s; shape checks only" % ORACLE)

    # \D{fmt} -- the only escape taking an argument. A literal in the format
    # proves the braces were consumed and not printed.
    got, ref = both(r"\D{ZZ%YZZ}")
    check(r"\D{fmt} runs strftime",
          bool(got) and re.match(r"^ZZ\d{4}ZZ$", got[-1] or ""),
          "got %r" % got[-3:])
    check(r"\D{fmt} agrees with bash", not ref or got[-1] == ref[-1],
          "hellish %r vs bash %r" % (got[-1:], ref[-1:]))

    # An empty format is bash's %X, not an empty prompt.
    got, ref = both(r"\D{}")
    check(r"\D{} falls back to the locale time",
          bool(got) and len(got[-1]) >= 5, "got %r" % got[-3:])

    # \T and \@ -- the 12-hour clocks.
    got, ref = both(r"\T")
    check(r"\T is the 12-hour clock",
          bool(got) and re.match(r"^\d\d:\d\d:\d\d$", got[-1] or ""),
          "got %r" % got[-3:])
    got, ref = both(r"\@")
    check(r"\@ is the 12-hour clock with am/pm",
          bool(got) and re.match(r"^\d\d:\d\d [AP]M$", got[-1] or "",
                                 re.I), "got %r" % got[-3:])

    # \! and \# -- both numbers, and \# must MOVE between prompts. A
    # constant would satisfy "is a number" and be useless.
    got, ref = both(r"\#", ["true", "true", "true"])
    nums = [g for g in got if g.isdigit()]
    check(r"\# is the command number and counts up",
          len(nums) >= 2 and int(nums[-1]) > int(nums[0]),
          "got %r" % got[-4:])
    got, ref = both(r"\!", ["true", "true"])
    check(r"\! is the history number",
          bool(got) and (got[-1] or "").isdigit(), "got %r" % got[-3:])

    # \l on a pty is the pts name; \V is the version.
    got, ref = both(r"\l")
    check(r"\l names the terminal",
          bool(got) and got[-1] and "\\" not in got[-1],
          "got %r" % got[-3:])
    got, ref = both(r"\V")
    check(r"\V is the version",
          bool(got) and re.match(r"^[0-9]", got[-1] or ""),
          "got %r" % got[-3:])

    # \r is a carriage return, so the text after it overwrites the text
    # before it -- there is nothing to see, only a byte to find.
    out = render(SHELL, r"a\rb")
    check(r"\r emits a carriage return", "|a\rb|" in out,
          "tail=%r" % out[-200:])

    # THE CONTROL. An escape bash does not know must still come out
    # literally in both -- that fallback is what these eight were wrongly
    # using, and it has to keep working for everything else.
    got, ref = both(r"\Z")
    check(r"an unknown escape is still literal",
          got and got[-1] == "\\Z" and (not ref or ref[-1] == "\\Z"),
          "hellish %r vs bash %r" % (got[-1:], ref[-1:]))

    # \A stays hellish's animation frame, on purpose. Pinning the
    # divergence is the point: bash renders HH:MM here.
    got, ref = both(r"\A")
    check(r"\A is hellish's, not bash's 24-hour clock -- known divergence",
          not ref or not re.match(r"^\d\d:\d\d$", got[-1] or "ZZ"),
          "hellish %r vs bash %r" % (got[-1:], ref[-1:]))

    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

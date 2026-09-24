#!/usr/bin/env python3
"""fc's listing, as an interactive bash prints it (issue #137).

fc matched its options as whole words -- `-l` and `-lr` only -- so every
other spelling of a listing fell into EDIT mode and started $EDITOR on the
history: `fc -ln -1` (which is how oh-my-zsh's sudo widget asks for the
previous command), `-nl`, `-lnr`, `-l -n`. It also counted the running fc
line itself as the most recent entry, so `-1` named fc, and it printed
`N<TAB>cmd` where bash prints `N<TAB> cmd`.

Every expectation below was produced by an interactive bash 5.2 on a pty,
from the same three input lines. One case is POSIX rather than bash:
`fc -l 3 1` lists 3 2 1 (first after last lists backwards). bash printed
only entry 1 in that exact position, and 3 2 1 one entry further on --
a quirk this does not copy.

History is interactive-only, so this needs a terminal; it waits on output,
not on the clock.

Usage: python3 fc_listing_tty_test.py [/path/to/hellish]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(HERE)
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

H1 = "1\t HISTFILE=/dev/null\n"
H2 = "2\t echo one\n"
H3 = "3\t echo two\n"
N1, N2, N3 = "\t HISTFILE=/dev/null\n", "\t echo one\n", "\t echo two\n"

# fc invocation, what it prints, its status.
CASES = [
    ("fc -ln -1", N3, 0),
    ("fc -nl -1", N3, 0),
    ("fc -l -1", H3, 0),
    ("fc -lr -3 -1", H3 + H2 + H1, 0),
    ("fc -lnr -2", N3 + N2, 0),
    ("fc -l -n -1", N3, 0),
    ("fc -ln", N1 + N2 + N3, 0),
    ("fc -l 1 2", H1 + H2, 0),
    ("fc -l", H1 + H2 + H3, 0),
    ("fc -ln 2", N2 + N3, 0),
    ("fc -rn -l -2", N3 + N2, 0),
    ("fc -l ech", H3, 0),
    ("fc -l 'echo o'", H2 + H3, 0),
    ("fc -l 3 1", H3 + H2 + H1, 0),
    ("fc -lr 1 3", H3 + H2 + H1, 0),
    ("fc -l -- -2", H2 + H3, 0),
    ("fc -l zzz", "", 1),
    ("fc -lx", "", 2),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def one(cmd, want, status):
    """A fresh shell per case, so entry numbers are fixed: 1 HISTFILE,
    2 echo one, 3 echo two, and the fc line itself is 4."""
    t = Tty(SHELL, env={"FCEDIT": "/bin/false", "EDITOR": "/bin/false"})
    t.expect(b"P$ ")
    for c in ("HISTFILE=/dev/null", "echo one", "echo two"):
        t.send(c + "\r")
        t.expect(b"P$ ")
    t.send("echo BEGIN; %s 2>/dev/null; echo \"END rc=$?\"\r" % cmd)
    t.expect(b"\nBEGIN\r\n")
    mark = t.pos
    ok = t.expect(b"END rc=")
    got = t.out[mark:t.pos - len(b"END rc=")].replace(b"\r", b"").decode()
    start = t.pos
    ok = ok and t.expect(b"\r\n")
    rc = t.out[start:t.pos - 2].decode() if ok else "?"
    t.close()
    check(cmd, ok and got == want and rc == str(status),
          "got %r rc=%s, want %r rc=%d" % (got, rc, want, status))


def main():
    for cmd, want, status in CASES:
        one(cmd, want, status)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

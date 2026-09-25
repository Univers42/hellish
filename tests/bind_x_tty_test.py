#!/usr/bin/env python3
"""bind -x: a key that runs a shell command, at a real prompt.

    bind -x '"\\C-xa": READLINE_LINE="echo from-x $READLINE_LINE"'

The command sees the line being edited as READLINE_LINE and the cursor as
READLINE_POINT (in characters, not bytes); what it leaves in them becomes
the line and the cursor, and whatever it prints appears on a clean row,
with the prompt and the line drawn again after it. fzf's key bindings for
bash are built on this. Every expectation below was checked against an
interactive bash 5.3.9 driven the same way.

Waits on output, not on the clock.

Usage: python3 bind_x_tty_test.py [/path/to/hellish]
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
RC = (r"""PS1='P$ '
bind -x '"\C-xa": READLINE_LINE="echo from-x $READLINE_LINE"; READLINE_POINT=0'
bind -x '"\C-xb": echo OUT-B'
bind -x '"\C-xc": echo "P=$READLINE_POINT M=$READLINE_MARK"'
""")
CX = "\x18"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def step(t, keys, want, name):
    mark = len(t.out)
    t.send(keys)
    ok = t.expect(want) and t.expect(b"P$ ")
    check(name, ok, repr(t.since(mark)[-300:]))


def main():
    t = Tty(SHELL, rc=RC, env={"LANG": "C.UTF-8"})
    check("start/prompt", t.expect(b"P$ "), repr(t.out[-300:]))
    # the command rewrote the line and put the cursor at 0: what is typed
    # next lands in front, so the line that runs is `echo echo from-x zz`
    step(t, "zz" + CX + "a" + "echo \r", b"\necho from-x zz\r\n",
         "line-and-point/come-back-from-the-command")
    # a command that prints, in the middle of a line: the output, then the
    # line as it was, still there to be run
    mark = len(t.out)
    t.send("echo keep" + CX + "b")
    check("output/is-shown", t.expect(b"OUT-B"), repr(t.since(mark)[-300:]))
    step(t, "\r", b"\nkeep\r\n", "output/the-line-survives-it")
    # READLINE_POINT counts characters: e-acute is two bytes, one character
    t.send(u"é12".encode("utf-8"))
    mark = len(t.out)
    t.send(CX + "c")
    check("point/is-in-characters", t.expect(b"P=3 M=0"),
          repr(t.since(mark)[-300:]))
    t.send("\x15")
    step(t, "echo \"[${READLINE_LINE-unset}${READLINE_POINT-}]\"\r",
         b"\n[unset]\r\n", "variables/exist-only-while-it-runs")
    st = t.close()
    check("exit/not-signalled", os.WIFEXITED(st), "wait status %r" % st)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

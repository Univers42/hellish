#!/usr/bin/env python3
"""No undo list leaks out of readline's history walk or its aborted search.

Two places where readline 8.1 (Ubuntu 22.04's) drops an undo list it owns,
each once per line, reported by LeakSanitizer when the shell exits:

  saved line  type, then walk the history with previous-history (Ctrl-P, or
              Up with prefix search off) and end the line on the recalled
              entry: Enter, ^C or ^G. The next read freed readline's saved
              copy of the typed line, but not its undo list.
              Fixed by hist_rl_reclaim_saved() (history_rl.c).
  search      Alt-P / Alt-N (stock non-incremental search), type a string,
              abort with ^G or Rubout on an empty string: readline restored
              the line over the search string's undo list.
              Fixed by rl_nsearch_reclaim() (rl_nsearch.c).

Each case is a fresh shell (in-process reader: the forked one discards its
heap with the child) run with detect_leaks=1; it must exit without a
report. The `works/` cases check the keys still do their job. On a binary
built without ASan nothing can be measured: SKIP, not pass.
"""
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else
                        os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
UP, DOWN = "\x1b[A", "\x1b[B"
ALT_P, ALT_N = "\x1bp", "\x1bn"
HISTORY = ["echo alpha", "echo beta"]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        FAILS.append(name)
        print("     " + detail)


def session(name, steps, want=None):
    """A shell with HISTORY run, then each key group of `steps` sent and
    given time to land, then exit. No leak report allowed; `want`, when
    given, must be printed by the line the keys ran."""
    t = Tty(SHELL, rc="PS1='P$ '\nHISTFILE=/dev/null\n",
            env={"ASAN_OPTIONS": "detect_leaks=1",
                 "HELLISH_HISTORY_PREFIX": "0"})
    ok = t.expect(b"P$ ")
    for c in HISTORY:
        t.send(c + "\r")
        ok = ok and t.expect(b"P$ ")
    mark = len(t.out)
    for keys in steps:
        t.send(keys)
        t._read(0.25)
    if want is not None:
        ok = ok and t.expect(b"\n" + want.encode() + b"\r\n")
    t.send("\x03")
    ok = ok and t.expect(b"P$ ")
    t.send("exit\r")
    # LeakSanitizer reports as the shell exits: read until the pty closes,
    # so the report is in t.out before anything is decided.
    end = time.time() + 10
    while t._read(0.2) and time.time() < end:
        pass
    st = t.close()
    leak = b"LeakSanitizer" in t.out
    check(name, ok and not leak and os.WIFEXITED(st),
          repr(t.since(mark)[-400:]))


def main():
    with open(SHELL, "rb") as f:
        if b"AddressSanitizer" not in f.read():
            print("SKIP %s has no ASan: leaks cannot be seen" % SHELL)
            return 0
    session("saved/up-then-enter", ["echo", UP, "\r"])
    session("saved/up-then-ctrl-c", ["echo", UP, "\x03"])
    session("saved/up-then-ctrl-g", ["ec", UP, "\x07"])
    session("saved/ctrl-p-then-enter", ["echo", "\x10", "\r"])
    session("saved/ctrl-p-walk-back-and-forth",
            ["echo", "\x10", "\x10", "\x0e", "\x10", "\r"])
    session("search/alt-p-abort-ctrl-g", [ALT_P, "ech", "\x07"])
    session("search/alt-n-abort-ctrl-g", [ALT_N, "ech", "\x07"])
    session("search/alt-p-rubout-past-empty", [ALT_P, "e", "\x7f", "\x7f"])
    session("search/typed-line-then-abort", ["ls", ALT_P, "ech", "\x07"])
    session("search/typed-then-alt-p-enter", ["ech", ALT_P, "\r"])
    session("works/up-down-brings-the-typed-line-back",
            ["echo typed", UP, DOWN, "\r"], want="typed")
    session("works/alt-p-search-runs-the-match",
            [ALT_P, "alph", "\r", "\r"], want="alpha")
    session("works/aborted-search-restores-the-line",
            ["echo kept", ALT_P, "x", "\x07", "\r"], want="kept")
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

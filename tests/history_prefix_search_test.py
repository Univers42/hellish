#!/usr/bin/env python3
"""Up searches the history for lines starting with what was typed (#136).

    ❯ git<Up>          ->  git push      (not whatever ran last)
    ❯ git<Up><Up>      ->  git status

zsh configs ask for it with `bindkey '^[[A' history-beginning-search-
backward` (or oh-my-zsh's up-line-or-beginning-search), bash ones with
`bind '"\\e[A": history-search-backward'`. Neither worked: a built-in
widget name bound the key to the shell-widget dispatcher, which found no
widget by that name and did nothing -- Up went dead -- and there was no
`bind` builtin at all. Both now reach readline's own history search,
bound directly, so repeated presses keep walking back (readline tracks the
search by checking the previous command was itself).

Each line's result is checked by RUNNING it: the four history entries
print four different words, and Enter shows which one the keys recalled.

History is interactive-only, so this needs a terminal; it waits on output,
not on the clock.

Usage: python3 history_prefix_search_test.py [/path/to/hellish]
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
UP, DOWN = "\x1b[A", "\x1b[B"
HISTORY = ["echo alpha", "printf 'beta\\n'", "echo gamma", "printf 'delta\\n'"]

BINDKEY = ("bindkey '^[[A' history-beginning-search-backward\n"
           "bindkey '^[[B' history-beginning-search-forward\n")
BIND = ("bind '\"\\e[A\": history-search-backward'\n"
        "bind '\"\\e[B\": history-search-forward'\n")
OMZ = ("bindkey '^[[A' up-line-or-beginning-search\n"
       "bindkey '^[[B' down-line-or-beginning-search\n")
WIDGET = ("my-up() { zle history-beginning-search-backward; }\n"
          "zle -N my-up\nbindkey '^[[A' my-up\n")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def session(rc, typed, want, name):
    """A fresh shell with `rc`, the four history lines, then `typed` and
    Enter: the recalled line must print `want`."""
    t = Tty(SHELL, rc="PS1='P$ '\nHISTFILE=/dev/null\n" + rc)
    ok = t.expect(b"P$ ")
    for c in HISTORY:
        t.send(c + "\r")
        ok = ok and t.expect(b"P$ ")
    mark = len(t.out)
    t.send(typed + "\r")
    ok = ok and t.expect(b"\n" + want.encode() + b"\r\n") and t.expect(b"P$ ")
    check(name, ok, repr(t.since(mark)[-300:]))
    st = t.close()
    check(name + "/exits-cleanly", os.WIFEXITED(st), "wait status %r" % st)


def main():
    session("", "echo" + UP, "delta", "control/no-binding-up-is-plain-history")
    session(BINDKEY, "echo" + UP, "gamma", "bindkey/up-finds-the-last-echo")
    session(BINDKEY, "echo" + UP + UP, "alpha", "bindkey/up-again-walks-back")
    session(BINDKEY, "echo" + UP + UP + DOWN, "gamma",
            "bindkey/down-walks-forward")
    session(BINDKEY, "printf" + UP + UP, "beta", "bindkey/another-prefix")
    session(BIND, "echo" + UP, "gamma", "bind/up-finds-the-last-echo")
    session(BIND, "echo" + UP + UP, "alpha", "bind/up-again-walks-back")
    session(OMZ, "echo" + UP + UP, "alpha", "omz-names/up-line-or-beginning")
    session(WIDGET, "echo" + UP, "gamma", "zle-builtin/called-from-a-widget")
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

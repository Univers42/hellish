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
import subprocess
import sys
import tempfile

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


READER = {}  # env of the reader under test (in-process / forked)


def session(rc, typed, want, name, env=None):
    """A fresh shell with `rc`, the four history lines, then `typed` and
    Enter: the recalled line must print `want`."""
    e = dict(READER)
    e.update(env or {})
    t = Tty(SHELL, rc="PS1='P$ '\nHISTFILE=/dev/null\n" + rc, env=e)
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


def run(name, rc, steps, env=None, hist=HISTORY, args=("-i",), bad=()):
    """A shell with `rc` and the `hist` lines run; then each (keys, wait)
    step: send `keys`, wait for the bytes `wait`. A step's wait is what
    proves what the keys did. `bad` bytes must never be printed."""
    e = dict(READER)
    e.update(env or {})
    t = Tty(SHELL, rc="PS1='P$ '\nHISTFILE=/dev/null\n" + rc, env=e,
            args=args)
    ok = t.expect(b"P$ ")
    for c in hist:
        t.send(c + "\r")
        ok = ok and t.expect(b"P$ ")
    for keys, wait in steps:
        t.send(keys)
        ok = ok and t.expect(wait)
    ok = ok and t.expect(b"P$ ")
    for b in bad:
        ok = ok and b not in t.out
    check(name, ok, repr(t.out[-300:]))
    t.close()


def out(word):
    return b"\n" + word.encode() + b"\r\n"


def edge():
    """Edge cases of the search itself."""
    run("edge/no-match-leaves-the-line", "",
        [("echo q" + UP + "\r", out("q"))])
    run("edge/prefix-equal-to-an-entry", "",
        [("echo gamma" + UP + UP + "\r", out("gamma"))])
    h3 = ["echo alpha", "echo;echo beta"]
    run("edge/trailing-space-is-part-of-prefix", "",
        [("echo " + UP + "\r", out("alpha"))], hist=h3)
    run("edge/no-trailing-space-matches-both", "",
        [("echo" + UP + "\r", out("beta"))], hist=h3)
    # readline matches on the text BEFORE the cursor ("echo "); the text
    # after it ("gam") is not part of the prefix and is replaced.
    run("edge/cursor-mid-line-uses-text-before-it", "",
        [("echo gam" + "\x1b[D" * 3 + UP + "\r", out("gamma"))],
        hist=["echo alpha", "echo gamma", "echo gamut-not-this"][:2]
        + ["printf 'delta\\n'"])
    run("edge/utf8-prefix", "",
        [("echo \u00e9" + UP + UP + "\r", out("\u00e91"))],
        hist=["echo \u00e91", "echo \u00fc2", "echo \u00e93"])
    run("edge/utf8-prefix-newest", "",
        [("echo \u00e9" + UP + "\r", out("\u00e93"))],
        hist=["echo \u00e91", "echo \u00fc2", "echo \u00e93"])
    run("edge/multiline-entry-by-prefix", "",
        [("for" + UP + "\r", b"\nL1\r\nL2\r\n")],
        hist=["echo a", "for i in 1 2\rdo echo L$i\rdone", "echo z"])
    run("edge/history-c-recalls-nothing", "",
        [("history -c\r", b"P$ "), ("echo hi" + UP + "\r", out("hi"))])
    run("edge/history-d-deleted-line-gone", "",
        [("history -d 3\r", b"P$ "), ("echo" + UP + "\r", out("alpha"))])
    run("edge/oldest-match-stays-put", "",
        [("echo" + UP * 3 + "\r", out("alpha"))])
    # readline's own behaviour: Down past the newest match rings the bell
    # and stays on that match (it does not go back to the typed prefix);
    # the cursor stays after the prefix, so " hi" lands inside the line.
    run("edge/down-past-newest-stays-on-newest-match", "",
        [("echo" + UP + DOWN + DOWN + " hi\r", out("hi gamma"))])
    run("edge/ctrl-c-mid-walk-restarts", "",
        [("echo" + UP + UP + "\x03", b"P$ "),
         ("echo" + UP + "\r", out("gamma"))])


def values():
    """Values of HELLISH_HISTORY_PREFIX, and where it is set."""
    up = [("echo" + UP + "\r", out("gamma"))]
    plain = [("echo" + UP + "\r", out("delta"))]
    run("val/uppercase-OFF-is-off", "HELLISH_HISTORY_PREFIX=OFF\n", plain)
    run("val/no-stays-on", "HELLISH_HISTORY_PREFIX=no\n", up)
    run("val/1-stays-on", "HELLISH_HISTORY_PREFIX=1\n", up)
    run("val/readonly-0-is-off", "declare -r HELLISH_HISTORY_PREFIX=0\n",
        plain, bad=(b"readonly", b"rror"))
    run("val/local-in-function-leaves-on",
        "f() { local HELLISH_HISTORY_PREFIX=0; :; }\nf\n", up,
        bad=(b"rror",))
    run("val/prompt-command-turns-off",
        "PROMPT_COMMAND='HELLISH_HISTORY_PREFIX=0'\n", plain)
    run("val/prompt-command-turns-on",
        "PROMPT_COMMAND='unset HELLISH_HISTORY_PREFIX'\n", up,
        env={"HELLISH_HISTORY_PREFIX": "0"})
    run("val/exported-env-0-then-unset-in-shell", "",
        [("unset HELLISH_HISTORY_PREFIX\r", b"P$ ")] + up,
        env={"HELLISH_HISTORY_PREFIX": "0"})


def modes():
    """Mode switches, user bindings, and other ways of starting."""
    up = [("echo" + UP + "\r", out("gamma"))]
    run("mode/vi-then-emacs-keeps-search", "set -o vi\n",
        [("set -o emacs\r", b"P$ ")] + up)
    run("mode/emacs-then-vi-keeps-search", "",
        [("set -o vi\r", b"P$ ")] + up)
    run("mode/vi-emacs-vi-emacs", "",
        [("set -o vi\r", b"P$ "), ("set -o emacs\r", b"P$ "),
         ("set -o vi\r", b"P$ ")] + up)
    run("mode/off-survives-vi-switch", "HELLISH_HISTORY_PREFIX=0\n",
        [("set -o vi\r", b"P$ "), ("printf 'omega\\n'\r", b"P$ "),
         ("echo" + UP + "\r", out("omega"))])
    run("user/bind-r-keeps-up-unbound", "bind -r '\\e[A'\n",
        [("true\r", b"P$ "), ("echo" + UP + " hi\r", out("hi"))])
    # An explicit `bind` of the stock function, typed at a prompt, is the
    # user's from then on: toggling the variable must never take it back.
    run("user/runtime-bind-previous-history-survives-toggling", "",
        [("bind '\"\\e[A\": previous-history'\r", b"P$ "),
         ("HELLISH_HISTORY_PREFIX=0\r", b"P$ "),
         ("unset HELLISH_HISTORY_PREFIX\r", b"P$ "),
         ("printf 'omega\\n'\r", b"P$ "),
         ("echo" + UP + "\r", out("omega"))])
    run("start/norc-gets-the-default", "", up,
        args=("--norc", "-i"), env={"PS1": "P$ "})
    run("start/login-shell-gets-the-default", "", up,
        args=("-l", "-i"), env={"PS1": "P$ "})


def noninteractive():
    """bind -p of a non-interactive shell stays bash's, byte for byte."""
    bash = os.path.expanduser("~/bash-5.3.9/bin/bash")
    cmd = "bind -p | grep -E 'history-search|previous-history|next-history'"
    env = dict(os.environ, LC_ALL="C.UTF-8", LANG="C.UTF-8")
    a = subprocess.run([SHELL, "-c", cmd], env=env, capture_output=True)
    if not os.path.exists(bash):
        print("skip noninteractive/bind-p-matches-bash (no oracle)")
        return
    b = subprocess.run([bash, "-c", cmd], env=env, capture_output=True)
    check("noninteractive/bind-p-matches-bash",
          a.stdout == b.stdout and a.stdout != b"", repr(a.stdout))


def toggle(name):
    """Runtime: default on, =0 -> plain, unset -> prefix again."""
    e = dict(READER)
    t = Tty(SHELL, rc="PS1='P$ '\nHISTFILE=/dev/null\n", env=e)
    ok = t.expect(b"P$ ")
    for c in HISTORY + ["HELLISH_HISTORY_PREFIX=0", "printf 'omega\\n'"]:
        t.send(c + "\r")
        ok = ok and t.expect(b"P$ ")
    t.send("echo" + UP + "\r")
    ok = ok and t.expect(b"\nomega\r\n") and t.expect(b"P$ ")
    t.send("unset HELLISH_HISTORY_PREFIX\r")
    ok = ok and t.expect(b"P$ ")
    t.send("echo" + UP + "\r")
    ok = ok and t.expect(b"\ngamma\r\n") and t.expect(b"P$ ")
    check(name, ok, repr(t.out[-300:]))
    t.close()


def matrix():
    """Every case, for the reader currently in READER."""
    session("", "echo" + UP, "gamma", "default/up-finds-the-last-echo")
    session("", "echo" + UP + UP, "alpha", "default/up-again-walks-back")
    session("", "echo" + UP + UP + DOWN, "gamma", "default/down-forward")
    session("", UP, "delta", "default/empty-line-up-is-plain")
    session("", "echo" + "\x1bOA", "gamma", "default/app-cursor-up")
    session("", "echo" + "\x1bOA\x1bOA\x1bOB", "gamma",
            "default/app-cursor-down")
    session("", "echo" + UP, "delta", "env0/plain",
            {"HELLISH_HISTORY_PREFIX": "0"})
    session("HELLISH_HISTORY_PREFIX=off\n", "echo" + UP, "delta",
            "rc-off/plain")
    session("HELLISH_HISTORY_PREFIX=\n", "echo" + UP, "gamma",
            "rc-empty/prefix")
    toggle("toggle/runtime")
    session("set -o vi\n", "echo" + UP, "gamma", "vi/insert-prefix")
    session("set -o vi\n", "echo" + "\x1bOA", "gamma", "vi/insert-app-up")
    rc = os.path.join(tempfile.mkdtemp(), "inputrc")
    with open(rc, "w") as f:
        f.write('"\\e[A": end-of-line\n')
    session("", "echo" + UP + " hi", "hi", "inputrc/binding-wins",
            {"INPUTRC": rc})
    session("bind '\"\\e[A\": previous-history'\n", "echo" + UP, "delta",
            "bind/previous-history-wins")
    session(BINDKEY, "echo" + UP, "gamma", "bindkey/up-finds-the-last-echo")
    session(BINDKEY, "echo" + UP + UP, "alpha", "bindkey/up-again-walks-back")
    session(BINDKEY, "echo" + UP + UP + DOWN, "gamma",
            "bindkey/down-walks-forward")
    session(BINDKEY, "printf" + UP + UP, "beta", "bindkey/another-prefix")
    session(BIND, "echo" + UP, "gamma", "bind/up-finds-the-last-echo")
    session(BIND, "echo" + UP + UP, "alpha", "bind/up-again-walks-back")
    session(OMZ, "echo" + UP + UP, "alpha", "omz-names/up-line-or-beginning")
    session(WIDGET, "echo" + UP, "gamma", "zle-builtin/called-from-a-widget")
    session(BIND, "echo" + UP, "gamma", "bind-explicit/survives-off",
            {"HELLISH_HISTORY_PREFIX": "0"})


def main():
    noninteractive()
    for cell, env in (("inproc", {}), ("fork", {"HELLISH_RL_FORK": "1"})):
        READER.clear()
        READER.update(env)
        print("== reader: " + cell)
        before = len(FAILS)
        matrix()
        edge()
        values()
        modes()
        for i in range(before, len(FAILS)):
            FAILS[i] = cell + ":" + FAILS[i]
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

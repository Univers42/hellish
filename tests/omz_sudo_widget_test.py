#!/usr/bin/env python3
"""ESC ESC, oh-my-zsh's sudo widget, end to end (issue #137).

    ╰─ ❯ hellish: ${${(Az)aliases[$cmd]}[1]:-$cmd}: bad substitution
    hellish: ${__hsh_nest_0[1]:-$cmd}: bad substitution

The framework installs omz's `sudo` plugin by default and it binds ESC ESC,
so pressing Escape twice on a fresh shell printed that. Four defects, found
one behind the other by pressing the key:

  1. `aliases` read empty: its WRITES reached the alias table (#114), its
     reads did not, so "what does this alias run" had no answer.
  2. `${${(...)...}[1]:-x}` -- a subscript AND an operator after a nested
     flagged expansion -- was a bad substitution, and so was a flag on a
     subscripted name, `${(z)aliases[ll]}`.
  3. `local new=$2` word-split $2: bash and zsh never split an assignment
     argument of a declaration builtin, hellish did for all but `export`,
     so `vim f` became `sudo/f` instead of `sudo -e f`.
  4. On an empty line the widget asks `$(fc -ln -1)` for the previous
     command; `-ln` was not a listing to fc, which opened $EDITOR instead.

The edit is checked by RUNNING the line with a fake sudo and vim on PATH
that print their arguments -- not by reading readline's redraw, which is
free to repaint the line however it likes.

The plugin comes from the same cache as plugin_corpus_test.py (downloaded
once, PLUGIN_CACHE / XDG_CACHE_HOME); with neither a cached copy nor a
network this SKIPS, out loud. Waits on output, never on the clock.

Usage: python3 omz_sudo_widget_test.py [/path/to/hellish]
"""
import os
import stat
import sys
import tempfile
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(HERE)
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
URL = ("https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/"
       "plugins/sudo/sudo.plugin.zsh")
CACHE = os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def plugin():
    """The cached plugin, fetched once. None means SKIP, not failure."""
    path = os.path.join(CACHE, "omz-sudo.zsh")
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return path
    if os.environ.get("OFFLINE") == "1":
        return None
    try:
        os.makedirs(CACHE, exist_ok=True)
        with urllib.request.urlopen(URL, timeout=20) as r:
            data = r.read()
        if not data:
            return None
        with open(path, "wb") as f:
            f.write(data)
        return path
    except (urllib.error.URLError, OSError, ValueError):
        return None


def fakes():
    """A bin dir whose sudo and vim only say what they were given."""
    d = tempfile.mkdtemp(prefix="sudo-widget-bin-")
    for name, tag in (("sudo", "FAKESUDO"), ("vim", "FAKEVIM")):
        p = os.path.join(d, name)
        with open(p, "w") as f:
            f.write('#!/bin/sh\necho "%s[$*]"\n' % tag)
        os.chmod(p, os.stat(p).st_mode | stat.S_IEXEC)
    return d


def line(t, keys, expect, name):
    """Type `keys` (a list: text, then the ESC ESC widget, then Enter) and
    expect `expect` from what the resulting line ran."""
    mark = len(t.out)
    for k in keys:
        t.send(k)
    ok = t.expect(expect) and t.expect(b"P$ ")
    check(name, ok, repr(t.since(mark)[-300:]))


def main():
    path = plugin()
    if not path:
        print("SKIP omz sudo plugin: no cached copy and no network")
        return 0
    rc = "PS1='P$ '\nEDITOR=vim\nexport EDITOR\n. %s\n" % path
    t = Tty(SHELL, rc=rc, path=fakes() + ":" + os.environ.get("PATH", ""))
    check("start/prompt", t.expect(b"P$ "), repr(t.out[-300:]))
    esc = b"\x1b\x1b"
    line(t, ["echo a1", esc, "\r"], b"FAKESUDO[echo a1]",
         "prefix/plain-command-gets-sudo")
    line(t, ["sudo echo a2", esc, "\r"], b"\r\na2\r\n",
         "toggle/sudo-comes-back-off")
    line(t, ["vim /tmp/f3", esc, "\r"], b"FAKESUDO[-e /tmp/f3]",
         "editor/becomes-sudo-e")
    line(t, ["sudo -e /tmp/f4", esc, "\r"], b"FAKEVIM[/tmp/f4]",
         "editor/sudo-e-goes-back-to-EDITOR")
    t.send("alias v=vim\r")
    t.expect(b"P$ ")
    line(t, ["v /tmp/f5", esc, "\r"], b"FAKESUDO[-e /tmp/f5]",
         "alias/of-the-editor-is-the-editor")
    t.send("echo a6\r")
    t.expect(b"a6\r\nP$ ")
    line(t, [esc, "\r"], b"FAKESUDO[echo a6]",
         "empty-line/recalls-the-previous-command")
    check("no-bad-substitution", b"bad substitution" not in t.out,
          repr(t.out[-400:]))
    st = t.close()
    check("exit/not-signalled", not os.WIFSIGNALED(st))
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

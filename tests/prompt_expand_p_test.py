#!/usr/bin/env python3
"""${PS1@P} is the prompt the shell actually shows (issue #134, section 1).

`prompt preview` rendered PS1 themes with `print -rP`. print -P is zsh's
and, as in zsh, scans what a variable expanded to for escapes again; the
PS1 renderer does not (bash's order). So a theme segment holding
`100%done` previewed as `100/home/...one`, a prompt that never appears.
${var@P}, bash's prompt-expansion operator, now runs the PS1 rule itself
and the preview uses it.

What this pins, at a real prompt:
  * the live PS1 and ${PS1@P} are the same bytes, in hellish's bilingual
    PS1 (a `%~` escape and a `\\W` escape side by side) and under
    `set -o zsh`, where PS1 is zsh's PROMPT and a variable's `%` IS read
    again -- @P follows the mode the prompt follows;
  * a variable's value is left as it is in the bilingual PS1;
  * `prompt preview` prints that same line for a PS1 theme.

The part of @P hellish shares with bash is graded against bash in
tests/scripts/61_prompt_expand_P.sh.

Cells: the in-process reader, and HELLISH_RL_FORK=1.

Usage: python3 prompt_expand_p_test.py [/path/to/hellish]
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(HERE)
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
SWITCH = os.path.join(ROOT, "share", "rc.d", "40-prompt-switch.hsh")
FAILS = []
T = 8.0
ROOT_USER = os.geteuid() == 0

# The value is what a theme segment gets from a hook: text with a percent
# and a backslash in it.
VALUE = r"100%done \w %d"
RC = ("V='%s'\n" % VALUE
      + "PS1='[${V}|%~|\\W|$((6*7))]\\$ '\n")
# HOME is the pty's temp dir and the shell starts in it: %~ and \W are ~.
LIVE = ("[%s|~|~|42]%s " % (VALUE, "#" if ROOT_USER else "$")).encode()


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def at_p(t, var):
    """Print ${var@P} between markers; return it, or None on a timeout."""
    t.send("printf '<%%s>\\n' \"${%s@P}\"\r" % var)
    mark = len(t.out)
    if not t.expect(b">\r\n", T):
        return None
    got = t.since(mark)
    start = got.rfind(b"\n<")
    return got[start + 2:-2] if start >= 0 else None


def live_prompt(t, suffix):
    """Run a command and return the prompt printed after it, through
    `suffix` (the prompt's known last bytes). The marker is typed as
    MA''RK so that its echo cannot be mistaken for its output."""
    t.send("echo MA''RK\r")
    if not t.expect(b"\nMARK\r\n", T):
        return None
    mark = t.pos
    if not t.expect(suffix, T):
        return None
    return t.out[mark:t.pos]


def bilingual_cell(cell, env):
    t = Tty(SHELL, rc=RC, env=env)
    try:
        ok = t.expect(LIVE, T)
        check("%s/live-prompt-leaves-the-value" % cell, ok,
              "tail=%r" % t.out[-160:])
        got = at_p(t, "PS1")
        check("%s/PS1@P-is-the-live-prompt" % cell, got == LIVE,
              "got=%r want=%r" % (got, LIVE))
        # A later change to the variable shows in both, the same way.
        t.send("V='50%off \\e'\r")
        want = LIVE.replace(VALUE.encode(), b"50%off \\e")
        live = live_prompt(t, want[-4:])
        check("%s/live-prompt-follows-the-variable" % cell, live == want,
              "live=%r want=%r" % (live, want))
        got = at_p(t, "PS1")
        check("%s/PS1@P-follows-the-variable" % cell, got == want,
              "got=%r want=%r" % (got, want))
    finally:
        t.close()


def zsh_cell(cell, env):
    """Under set -o zsh, PS1 is zsh's PROMPT: the value is substituted
    first and its `%` IS an escape (zsh's PROMPT_SUBST order). @P follows
    the prompt there too."""
    rc = "V='50%~x'\nset -o zsh\nPS1='[${V}]%# '\n"
    t = Tty(SHELL, rc=rc, env=env)
    try:
        suffix = b"]# " if ROOT_USER else b"]% "
        live = live_prompt(t, suffix)
        check("%s/zsh-live-prompt-reads-the-value" % cell,
              live == b"[50~x" + suffix,
              "live=%r" % live)
        got = at_p(t, "PS1")
        check("%s/zsh-PS1@P-is-the-live-prompt" % cell, got == live,
              "got=%r live=%r" % (got, live))
    finally:
        t.close()


def preview_cell(cell, env):
    """prompt preview prints, for a PS1 theme, the line the prompt shows
    once the theme is active."""
    t = Tty(SHELL, rc=RC, env=env)
    tdir = os.path.join(t.home, "themes")
    os.mkdir(tdir)
    with open(os.path.join(tdir, "pct.hsh"), "w") as f:
        f.write("PS1='[${V}|%~|\\W|$((6*7))]\\$ '\n")
    try:
        t.expect(LIVE, T)
        t.send("HELLISH_THEMES=%s; . %s; prompt preview; echo DO''NE\r"
               % (tdir, SWITCH))
        mark = len(t.out)
        ok = t.expect(b"\nDONE\r\n", T)
        out = t.since(mark)
        line = b"\npct            " + LIVE + b"\n"
        check("%s/preview-is-the-live-prompt" % cell, ok and line in out,
              "out=%r want=%r" % (out[-200:], line))
    finally:
        t.close()


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    for cell, env in (("inproc", {}), ("fork", {"HELLISH_RL_FORK": "1"})):
        bilingual_cell(cell, env)
        zsh_cell(cell, env)
        preview_cell(cell, env)
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

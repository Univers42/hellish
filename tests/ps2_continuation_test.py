#!/usr/bin/env python3
"""PS2 is the prompt of every continuation line (issue #134, section 4).

    $ PS2='CUSTOM2> '
    $ echo one \\
    > two            # still '> ', not CUSTOM2>

PS2 was read only for an unfinished compound command (`if`, a function
body, a pipe). The other continuation reads handed readline a fixed
label -- `> ` after a backslash-newline, `dquote> ` / `squote> ` in an
open quote, `subshell> ` / `bquote> ` in an open substitution,
`heredoc> ` for a here-document body -- so a PS2 set in an rc looked
ignored. In bash PS2 is the one continuation prompt; prompt_ps2 now
decides it for every one of those reads.

What each part pins, at a real prompt:
  * with PS2 set, every kind of continuation line shows it, rendered
    (a variable and an arithmetic expansion in it), and the command still
    runs; a change to the variable shows on the next continuation line;
  * `unset PS2` brings the built-in labels back;
  * no right prompt on a continuation row: the lexer reads skipped the
    reset the compound path did, so RPROMPT was painted after `dquote> `.

That PS2 is never expanded when no prompt is shown (a script, a pipe) is
graded against bash in tests/scripts/59_ps2_not_expanded.sh.

Cells: the in-process reader, and HELLISH_RL_FORK=1. Waits on output: a
missing prompt is a timeout and a named failure, not a sleep.

Usage: python3 ps2_continuation_test.py [/path/to/hellish]
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
T = 8.0

# name, the lines typed, what the command prints. Every line but the last
# is left open, so each one must be answered by a continuation prompt.
CASES = [
    ("backslash-newline", ["echo B1 \\", "B2"], b"B1 B2\r\n"),
    ("double quote", ['echo "D1', 'D2"'], b"D1\r\nD2\r\n"),
    ("single quote", ["echo 'S1", "S2'"], b"S1\r\nS2\r\n"),
    ("heredoc body", ["cat <<E", "H1", "E"], b"H1\r\n"),
    ("command substitution", ["echo $(", "echo C1)"], b"C1\r\n"),
    ("backquote", ["echo `", "echo Q1`"], b"Q1\r\n"),
    ("if", ["if true", "then echo I1", "fi"], b"I1\r\n"),
    ("brace group", ["{", "echo G1; }"], b"G1\r\n"),
    ("pipe", ["echo P1 |", "cat"], b"P1\r\n"),
    ("and-list", ["true &&", "echo A1"], b"A1\r\n"),
]

# The built-in labels, with PS2 unset.
LABELS = [
    ("backslash-newline", b"> "),
    ("double quote", b"dquote> "),
    ("heredoc body", b"heredoc> "),
    ("if", b"if> "),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def run(t, lines, cont, out, cell, name):
    """Type `lines`; every line but the last must get `cont` back. A miss
    is reported and the rest is still typed, so the shell gets back to
    its primary prompt and the next case runs. Returns the offsets (first
    continuation prompt, output), or None when anything was missed."""
    first = None
    shown = True
    for i, line in enumerate(lines):
        t.send(line + "\r")
        if i == len(lines) - 1 or not shown:
            continue
        shown = t.expect(cont, T)
        if first is None:
            first = t.pos
    check("%s: %s: continuation lines show %r" % (cell, name, cont), shown,
          repr(t.since(max(0, len(t.out) - 160))))
    at = t.pos
    ok = t.expect(out, T)
    check("%s: %s: runs" % (cell, name), ok,
          repr(t.since(at)[-160:]))
    end = t.pos - len(out)
    ok = t.expect(b"P$ ", T) and ok
    check("%s: %s: back at the primary prompt" % (cell, name), ok)
    return (first, end) if ok and shown else None


def ps2_cell(cell, env):
    t = Tty(SHELL, rc="PS1='P$ '\nK=c2\nPS2='${K}$((1+1))> '\n", env=env)
    try:
        if not t.expect(b"P$ ", T):
            return check(cell + ": first prompt", False)
        for name, lines, out in CASES:
            run(t, lines, b"c22> ", out, cell, name)
        t.send("K=later\r")
        t.expect(b"P$ ", T)
        run(t, ['echo "L1', 'L2"'], b"later2> ", b"L1\r\nL2\r\n", cell,
            "PS2 re-rendered for each continuation line")
        t.send("unset PS2\r")
        t.expect(b"P$ ", T)
        for name, label in LABELS:
            lines, out = next((c[1], c[2]) for c in CASES if c[0] == name)
            run(t, lines, label, out, cell, "unset PS2: " + name
                + " is " + label.decode().strip())
    finally:
        t.close()


def rprompt_cell(cell, env):
    env = dict(env, TERM="xterm")
    t = Tty(SHELL, rc="PS1='P$ '\nRPROMPT=RPX\n", env=env)
    try:
        if not t.expect(b"P$ ", T):
            return check(cell + ": rprompt: first prompt", False)
        check(cell + ": rprompt: painted on the primary prompt",
              t.expect(b"RPX", T))
        for name, cont in (("double quote", b"dquote> "),
                           ("backslash-newline", b"> "),
                           ("heredoc body", b"heredoc> "),
                           ("if", b"if> ")):
            lines, out = next((c[1], c[2]) for c in CASES if c[0] == name)
            span = run(t, lines, cont, out, cell, "rprompt: " + name)
            if span is None:
                continue
            row = t.out[span[0]:span[1]]
            check("%s: rprompt: none on a %s continuation row"
                  % (cell, name), b"RPX" not in row, repr(row[-160:]))
    finally:
        t.close()


def main():
    if not os.access(SHELL, os.X_OK):
        print("hellish not built at " + SHELL)
        return 1
    for cell, env in (("inproc", {}), ("fork", {"HELLISH_RL_FORK": "1"})):
        ps2_cell(cell, env)
        rprompt_cell(cell, env)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

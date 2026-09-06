#!/usr/bin/env python3
"""Regression test: `select` renders bash's menu byte for byte -- issue #122.

`select o in a b; do ...; done` was a syntax error.  The golden suite pins
what reaches stdout and the exit status; this test pins the part the golden
harness deliberately ignores -- STDERR -- because the menu IS the feature:
bash lays the numbered words out column-major in cells of one width, fills
the gaps with TABS wherever a tab stop falls inside them (print_select_list
+ indent(), execute_cmd.c), widens the first column's index field to the
row count and the others to the item count, measures words with wcswidth in
a multibyte locale, and prints PS3 after every menu.  A menu that "looks
the same" and differs by a tab would break every script that scrapes it.

Each case runs the same script through hellish and bash with the same
stdin and environment and requires stdout, stderr and the exit status to be
identical.  The oracle is the pinned bash 5.3.9 when present (HELLISH_ORACLE
or ~/bash-5.3.9), else whatever bash is on PATH -- the layout code has not
changed across the versions that matter.

Usage: python3 select_menu_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ORACLE = os.environ.get("HELLISH_ORACLE") or os.path.expanduser(
    "~/bash-5.3.9/bin/bash")
if not os.path.isfile(ORACLE):
    ORACLE = "bash"
BASE_ENV = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1"}
FAILS = []

TWELVE = "alpha beta gamma delta epsilon zeta eta theta iota kappa lambda mu"
CASES = [
    ("three items, one per line",
     {"COLUMNS": "80"}, "select o in a b c; do :; done", b""),
    ("twelve items in 40 columns: tabs inside the gaps",
     {"COLUMNS": "40"}, "select o in %s; do :; done" % TWELVE, b""),
    ("twelve items in 80 columns",
     {"COLUMNS": "80"}, "select o in %s; do :; done" % TWELVE, b""),
    ("a hundred items: three-digit indices, first column narrower",
     {"COLUMNS": "80"}, "select o in $(seq 1 100); do :; done", b""),
    ("a width too small for any gap",
     {"COLUMNS": "20"}, "select o in $(seq 1 30); do :; done", b""),
    ("multibyte words measured by display width",
     {"COLUMNS": "30", "LC_ALL": "C.UTF-8"},
     "select o in café naïve über a b c d e f g; do :; done",
     b""),
    ("double-width words",
     {"COLUMNS": "60", "LC_ALL": "C.UTF-8"},
     "select o in 日本語 x 中文字 y z w; do :; done",
     b""),
    ("PS3 replaces the prompt",
     {"COLUMNS": "80"}, 'PS3="pick> "; select o in a; do :; done', b""),
    ("words with spaces stay one item",
     {"COLUMNS": "80"}, "select o in \"a b\" 'c  d' e; do :; done", b""),
    ("no COLUMNS and no terminal: 80",
     {}, "select o in %s; do :; done" % TWELVE, b""),
    ("an empty line reprints the menu, a choice runs the body",
     {"COLUMNS": "80"},
     'select o in a b; do echo "[$o][$REPLY]"; done; echo "rc=$?"',
     b"\n2\nx\n1\n"),
    ("EOF: a newline on stdout, status 1, menu printed once",
     {"COLUMNS": "80"},
     'select o in a b; do echo never; done; echo "rc=$?"', b""),
    ("break leaves without a newline",
     {"COLUMNS": "80"},
     'select o in a b; do echo "[$o]"; break; done; echo "rc=$?"', b"2\n"),
]


def run(shell, extra, script, stdin):
    env = dict(BASE_ENV)
    env.update(extra)
    return subprocess.run([shell, "-c", script], input=stdin,
                          capture_output=True, env=env, timeout=30)


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def first_diff(a, b):
    la, lb = a.split(b"\n"), b.split(b"\n")
    for i, (x, y) in enumerate(zip(la, lb)):
        if x != y:
            return "line %d: hellish=%r bash=%r" % (i + 1, x, y)
    return "lengths %d vs %d" % (len(a), len(b))


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    for name, extra, script, stdin in CASES:
        h = run(SHELL, extra, script, stdin)
        b = run(ORACLE, extra, script, stdin)
        detail = ""
        if h.stderr != b.stderr:
            detail = "stderr " + first_diff(h.stderr, b.stderr)
        elif h.stdout != b.stdout:
            detail = "stdout " + first_diff(h.stdout, b.stdout)
        elif h.returncode != b.returncode:
            detail = "rc %d vs %d" % (h.returncode, b.returncode)
        check(name, detail == "", detail)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""The pattern matcher answers "no match" in one pass, not in n^2 of them.

Found on a fresh install.  bash-preexec's __bp_sanitize_string runs one
global substitution over PROMPT_COMMAND,

    ${sanitized//?(+([[:blank:]]))[";$nl"]*([[:blank:]]):.../$nl}

which normally matches nothing at all, and it cost 7.7 seconds at 122
characters where bash spends none.  Every prompt hook a framework installs
goes through it, so the first thing a new machine did was stall for
several seconds per shell -- and under CI load the pty gate watching that
install timed out waiting for a prompt that was still busy matching.

Two compounding costs, both pinned here:

  * ${v//p/r} asked "does p match here" at every position and, for each,
    tried every length -- n^2 whole-pattern matches to discover that the
    pattern matches nowhere.  bash wraps the pattern in `*`s and asks once;
    if `*p*` does not match, no substring does.
  * every one of those matches COPIED what it was about: an extglob
    alternative, the prefix a group was asked to consume.  A slice needs
    no copy.

Both are complexity, not constants, so the checks are shaped as work that
is instant when the fix is in and effectively unbounded when it is not --
the first case below took over an hour before it.  The limits are wall
clock and deliberately loose: a machine ten times slower than the one this
was written on still passes with two orders of magnitude to spare.

The matching itself is pinned elsewhere -- tests/pattern_slice against the
bash oracle, and multibyte_test.py for the part of the bound that is in
bytes while a character is not.

Usage: python3 pattern_cost_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

ENV = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
       "HOME": os.environ.get("HOME", "/tmp"), "TERM": "dumb",
       "LC_ALL": "C", "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_BANNER": "0",
       "ASAN_OPTIONS": "detect_leaks=0"}

# (name, seconds allowed, expected stdout, script)
CASES = [
    ("a bracket that matches nothing, over 20k characters", 20.0, "20000\n",
     'v=$(printf "a%.0s" $(seq 1 20000)); r="${v//[b]/X}"; echo "${#r}"\n'),

    ("...and the same with the match at the very end", 20.0, "20000\n",
     'v=$(printf "a%.0s" $(seq 1 19999))b; r="${v//[b]/X}"; echo "${#r}"\n'),

    ("a * pattern that matches nothing, over 20k characters", 20.0,
     "20000\n",
     'v=$(printf "a%.0s" $(seq 1 20000)); r="${v//x*y/X}"; echo "${#r}"\n'),

    ("the #, ##, %% trims over the same 20k characters", 20.0,
     "20000 20000 20000\n",
     'v=$(printf "a%.0s" $(seq 1 20000))\n'
     'a="${v#[b]}"; b="${v##[b]}"; c="${v%%[b]}"\n'
     'echo "${#a} ${#b} ${#c}"\n'),

    ("bash-preexec's own sanitiser, at a realistic PROMPT_COMMAND", 20.0,
     "ok\n",
     'shopt -s extglob\n'
     'nl=$(printf "\\nx"); nl=${nl%x}\n'
     'pad=$(printf "x%.0s" $(seq 1 90))\n'
     'v="_hx_precmd_run$pad${nl}__bp_install \\"\\$_\\""\n'
     'eval \'i=0; while [ $i -lt 5 ]; do\n'
     '  t="${v//?(+([[:blank:]]))[";$nl"]*([[:blank:]]):'
     '*([[:blank:]])[";$nl"]*([[:blank:]])/$nl}"\n'
     '  i=$((i + 1)); done\'\n'
     '[ "$t" = "$v" ] && echo ok || echo "changed: $t"\n'),
]


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m",
                       name), flush=True)
    if not ok:
        if detail:
            print("       %s" % detail.replace("\n", "\n       "))
        FAILS.append(name)


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    print("\033[1;36m>\033[0m \033[1ma pattern that matches nothing is "
          "cheap to refuse\033[0m", flush=True)
    for name, budget, want, script in CASES:
        start = time.time()
        try:
            p = subprocess.run([SHELL], input=script, env=ENV,
                               capture_output=True, text=True,
                               timeout=budget)
            out, rc = p.stdout, p.returncode
        except subprocess.TimeoutExpired:
            check(name, False, "still running after %.0fs" % budget)
            continue
        took = time.time() - start
        check(name, out == want and rc == 0,
              "out=%r rc=%d want=%r (%.1fs)" % (out, rc, want, took))
        if out == want and rc == 0:
            print("       %.2fs of %.0fs" % (took, budget))
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

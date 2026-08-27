#!/usr/bin/env python3
"""Regression test: a config can find out what it defined -- issue #71 item 2.

`declare -F` and `declare -f` both returned NOTHING, always, and exited 0
while doing it:

    $ hellish -c 'f(){ :; }; declare -F; echo END'
    END

declare_scan() recognised only p/x/A/n/i, so -F and -f were swallowed as
no-op option words and the assignment loop ran zero times. Silent success.

Why it matters: every mature shell config has a `help`-style command that
tells you what it gave you, and in bash you build that by introspection.
Here it could not be built at all -- the reporter had to make every module
hand-register its aliases and functions into a registry maintained by hand,
so a plugin that forgets to register is invisible. It also blocks the other
thing a plugin manager needs: detecting that two plugins both define `gs`.

`declare -f` (BODIES) is deliberately NOT implemented and says so on stderr
with a non-zero status. There is no AST deparser in the tree, and rebuilding
source text from token spans is not safe (t_token.start only points into the
input buffer while t_token.allocated is false). Printing an inexact body
would repeat the exact failure mode issue #71 item 4 complains about -- a
feature that reports success and is subtly wrong. This test pins the loud
failure, so that if bodies land later, it is a deliberate change.

Usage: python3 declare_introspect_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(script):
    return subprocess.run([SHELL, "-c", script], capture_output=True,
                          text=True, env=ENV, timeout=30)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    # 1. bare -F lists every function, one `declare -f NAME` line each.
    p = run('a() { :; }; b() { :; }; declare -F')
    lines = sorted(p.stdout.split("\n")[:-1])
    check("declare -F lists defined functions",
          lines == ["declare -f a", "declare -f b"],
          "got %r stderr=%r" % (lines, p.stderr.strip()[:120]))

    # 2. -F with a name: prints the bare name, status 0.
    p = run('a() { :; }; declare -F a; echo "rc=$?"')
    check("declare -F NAME prints the name and succeeds",
          p.stdout.split() == ["a", "rc=0"], "got %r" % (p.stdout.split(),))

    # 3. -F with an undefined name: no output, status 1. This is the
    #    existence test a plugin manager branches on.
    p = run('declare -F nope; echo "rc=$?"')
    check("declare -F on an undefined name reports 1",
          p.stdout.split() == ["rc=1"], "got %r" % (p.stdout.split(),))

    # 4. the conflict-detection case that motivated the issue.
    #    Both directions -- a test that only checks the positive passes even
    #    on the broken build, where -F always exited 0.
    p = run('gs() { :; }; '
            'declare -F gs  >/dev/null && echo TAKEN   || echo free; '
            'declare -F zzz >/dev/null && echo WRONG   || echo free2')
    check("a plugin can detect that a name is already taken",
          p.stdout.split() == ["TAKEN", "free2"],
          "got %r" % (p.stdout.split(),))

    # 5. no functions at all -> nothing, status 0, no crash.
    p = run('declare -F; echo "rc=$?"')
    check("declare -F with no functions is empty and succeeds",
          p.stdout.split() == ["rc=0"], "got %r" % (p.stdout.split(),))

    # 6. unset removes it from the listing.
    p = run('a() { :; }; b() { :; }; unset -f a; declare -F')
    check("unset -f removes a function from declare -F",
          p.stdout.split("\n")[:-1] == ["declare -f b"],
          "got %r" % (p.stdout.split("\n")[:-1],))

    # 7. -f is a LOUD not-implemented, never a silent success.
    p = run('a() { :; }; declare -f a; echo "rc=$?"')
    check("declare -f fails loudly instead of silently printing nothing",
          p.returncode != 0 or "rc=0" not in p.stdout,
          "declare -f exited 0 with no output -- the original bug")
    check("...and says why on stderr",
          "not implemented" in p.stderr, "stderr=%r" % p.stderr.strip()[:150])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

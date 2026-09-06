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

`declare -f` prints the definition, rebuilt from the body's SOURCE TEXT
(src/infrastructure/ast_span.c). The wrapper `name () { ... }` is synthesised
rather than taken from source, because `{`, `}` and `()` are not AST tokens:
a span over the whole definition stops at the last WORD and comes back as
`f() { echo hi;` with no closing brace, which does not re-parse. The one
thing this output must do is survive a round trip through eval, so that is
what the test asserts.

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

    # 7. -f prints a body, and it must be one the shell can read back.
    p = run('a() { echo hi; }; declare -f a')
    check("declare -f prints the definition",
          "echo hi" in p.stdout and p.stdout.strip().endswith("}"),
          "got %r -- a truncated body does not re-parse" % p.stdout.strip())

    #    The property that matters: round-trip. A body missing its closing
    #    brace still "looks right" in a terminal and is useless.
    p = run('a() { echo ROUNDTRIP; }; b=$(declare -f a); unset -f a; '
            'eval "$b"; a')
    check("declare -f output survives a round trip through eval",
          p.stdout.strip() == "ROUNDTRIP",
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    #    multi-line bodies too, not just one-liners
    p = run('m() {\n  local x=1\n  echo "m$x"\n}\n'
            'b=$(declare -f m); unset -f m; eval "$b"; m')
    check("a multi-line body round-trips", p.stdout.strip() == "m1",
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    #    an unknown name is still status 1, not a crash
    # A body whose FIRST statement opens with a keyword the parser consumes
    # (for, select, while, if, case, {, (, !) used to print without it --
    # `x in a; do :; done;` -- which does not re-parse (issue #122 found it
    # through `select`).  The span is now pinned to the text between the
    # braces, so every one of these comes back exactly as written and
    # survives eval.
    for body in ["for x in a; do echo $x; done",
                 "select x in a; do echo $x; break; done",
                 "while false; do :; done",
                 "until true; do :; done",
                 "if true; then echo t; fi",
                 "case a in a) echo ca;; esac",
                 "{ echo grp; }",
                 "( echo sub )",
                 "! false",
                 "(( 1 ))",
                 "coproc :"]:
        p = run("f() { %s; }; declare -f f" % body)
        check("declare -f keeps the leading keyword of: " + body,
              p.stdout.split("\n")[2] == body + ";",
              "got %r" % p.stdout.split("\n")[:4])
    p = run('f() { for x in a b; do echo $x; done; }; b=$(declare -f f); '
            'unset -f f; eval "$b"; f')
    check("a for body round-trips through eval",
          p.stdout.split() == ["a", "b"], "got %r" % p.stdout)
    p = run('f() {\n  echo hi\n  echo yo\n}\ndeclare -f f')
    check("a multi-line body keeps its lines and gains no blank ones",
          p.stdout == "f ()\n{\necho hi\n  echo yo\n}\n",
          "got %r" % p.stdout)
    p = run('f() ( echo sub ); declare -f f')
    check("a subshell body prints whole, parens included",
          p.stdout.split("\n")[2] == "( echo sub )",
          "got %r" % p.stdout.split("\n")[:4])

    p = run('declare -f nope; echo "rc=$?"')
    check("declare -f on an undefined name reports 1",
          p.stdout.split() == ["rc=1"], "got %r" % (p.stdout.split(),))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

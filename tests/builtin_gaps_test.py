#!/usr/bin/env python3
"""Regression test: `local -n`, `alias -p`, `dirs` -- issue #71 items 5.3,
2 and 5.12.

`local -n` is the dangerous one, and the issue rates it worse than
unimplemented for the right reason:

    builtin_local did NO option parsing at all. It started at argv[1] and
    strdup'd every word as a variable name, so `local -n ref=var` created a
    shell variable literally called "-n" and bound `ref` to the STRING
    "var". Silently, with status 0.

A nameref that yields the name instead of the value corrupts data rather
than failing, and `declare -n` -- which already worked, through the same
env_attr table -- made it look supported. `local` simply never reached it.

`alias -p` was "alias: -p: not found". Note bare `alias` and `alias -p`
print DIFFERENT things in bash, and both forms here are checked against the
pinned 5.3.9 oracle: bare prints `name='value'` (byte-for-byte what the
golden suite diffs), -p prefixes `alias ` so the output can be pasted back
into an rc file.

`dirs` was missing while pushd and popd both shipped. The printer already
existed and pushd/popd were already calling it; it was just static.

Usage: python3 builtin_gaps_test.py [/path/to/hellish]
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

    # ---- local -n -------------------------------------------------------
    p = run('v=VALUE; f() { local -n r=v; echo "[$r]"; }; f')
    check("local -n dereferences the target, not its name",
          p.stdout.strip() == "[VALUE]",
          "got %r -- the name instead of the value is the corruption bug"
          % p.stdout.strip())

    # the smoking gun: a variable literally called "-n"
    p = run('v=1; f() { local -n r=v; }; f; set | grep -c -- "^-n=" || true')
    check("local -n does not create a variable called '-n'",
          "1" not in p.stdout.split(), "got %r" % p.stdout.strip())

    # options must not become variable names, even the ones we only consume
    p = run('f() { local -r x=1; echo "[$x]"; }; f')
    check("an accepted-and-ignored option is not treated as a name",
          p.stdout.strip() == "[1]", "got %r" % p.stdout.strip())

    # and it must still be LOCAL -- the attribute has to unwind
    p = run('v=A; w=B; g() { local -n r=w; echo "in=$r"; }; '
            'f() { local -n r=v; g; echo "back=$r"; }; f')
    check("the nameref binding unwinds when the function returns",
          p.stdout.split() == ["in=B", "back=A"],
          "got %r" % (p.stdout.split(),))

    # plain local is untouched
    p = run('x=OUT; f() { local x=IN; echo "$x"; }; f; echo "$x"')
    check("plain local still works", p.stdout.split() == ["IN", "OUT"],
          "got %r" % (p.stdout.split(),))

    # ---- alias -p -------------------------------------------------------
    p = run("alias ll='ls -l'; alias -p")
    check("alias -p prints the reusable form",
          p.stdout.strip() == "alias ll='ls -l'",
          "got %r stderr=%r" % (p.stdout.strip(), p.stderr.strip()[:100]))
    p = run("alias ll='ls -l'; alias")
    check("bare alias is unchanged (the golden suite diffs it byte-for-byte)",
          p.stdout.strip() == "ll='ls -l'", "got %r" % p.stdout.strip())
    p = run("alias ll='ls -l'; alias -p | . /dev/stdin; alias")
    check("alias -p output can be sourced back",
          p.stdout.strip().endswith("ll='ls -l'"),
          "got %r" % p.stdout.strip())

    # ---- dirs -----------------------------------------------------------
    p = run('cd /tmp; pushd /usr >/dev/null; pushd /etc >/dev/null; dirs')
    check("dirs prints the stack, deepest last",
          p.stdout.strip() == "/etc /usr /tmp",
          "got %r stderr=%r" % (p.stdout.strip(), p.stderr.strip()[:100]))
    p = run('cd /tmp; pushd /usr >/dev/null; dirs -c; dirs')
    check("dirs -c empties the stack", p.stdout.strip() == "/usr",
          "got %r" % p.stdout.strip())
    p = run('dirs -z; echo "rc=$?"')
    check("dirs rejects an unknown option loudly",
          "rc=2" in p.stdout and "invalid option" in p.stderr,
          "got %r / %r" % (p.stdout.strip(), p.stderr.strip()[:100]))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

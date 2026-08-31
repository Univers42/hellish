#!/usr/bin/env python3
"""`local` used to leave its bookkeeping behind, and only one build could see.

THE BUG. scope_leave restores each saved variable and drops the vec's
LENGTH; nothing ever released the vec's backing buffer. So the first `local`
in a session put ~80 bytes on the heap and left them there until exit.

WHY IT HID. It is reachable through t_shell for the whole session, so
LeakSanitizer -- which reports what is LOST, not what is merely never freed
-- says nothing about it on the default build. And the golden suite does not
weigh anything. It shows up only on the ft_malloc oracle
(`HELLISH_ALLOC_STATS=1` on a SAFE=0 build), which prints the bytes still
live after free_all_state has run. That is the same instrument that caught
the 18 KB alias leak and the 17 KB ZLE tables, and this is the same shape:
a container whose destructor was never written.

It was found from the other end. Sourcing zsh-autosuggestions left exactly
80 bytes on the oracle, which bisected to `local` inside the plugin's
anonymous function -- and then reproduced with a plain named function in a
plain `.sh` file, which is how it was established as pre-existing rather
than something the anonymous-function work had introduced.

HOW THIS TESTS IT. Not against zero: the shell has a fixed allocation floor
(~4.5 KB of session structures the oracle counts) that says nothing about
any one feature. The claim is that `local` costs NOTHING ON TOP of a session
that ran a bare `true` -- and that a thousand calls cost the same as one, so
a per-call regression cannot hide inside the floor either.

THE SECOND CASE IS THE ONE THAT BIT. A shell that exits from INSIDE a
function still holds unrestored saves, and the destructor has to free them
rather than restore them -- restoring writes into an environment that is
about to be freed, and hands ownership of the same strings to env_set. The
first version of that loop indexed the vec after decrementing its length and
segfaulted on `f() { local v=1; exit 0; }`, which no other test in the tree
executes. It is asserted here on both the crash and the exit STATUS, because
a shell that dies during teardown can still have printed everything the
caller expected.

Usage: python3 alloc_scope_test.py [/path/to/hellish]
       needs a SAFE=0 build to measure; SKIPS cleanly otherwise.
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ENV = dict(os.environ, HELLISH_ALLOC_STATS="1", HELLISH_NO_BANNER="1",
           HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(script):
    """Run a script FILE and return (rc, stdout+stderr)."""
    fd, path = tempfile.mkstemp(suffix=".sh")
    with os.fdopen(fd, "w") as f:
        f.write(script + "\n")
    try:
        p = subprocess.run([SHELL, path], capture_output=True, text=True,
                           timeout=120, env=ENV)
        return p.returncode, p.stdout + p.stderr
    finally:
        os.unlink(path)


def live(script):
    """Bytes still live after cleanup, or None when this build cannot say."""
    _, out = run(script)
    m = re.search(r"live bytes after cleanup: (\d+)", out)
    if not m:
        return None
    return int(m.group(1))


def main():
    if not os.path.isfile(SHELL):
        print("no shell at", SHELL)
        return 1

    base = live("true")
    if base is None:
        print("skip: this build does not report allocator stats "
              "(needs SAFE=0 -- `make MODE=release` or OPT=1)")
        return 0
    print("--- floor for a session that did nothing: %d bytes" % base)

    for name, script in (
        ("one local in a named function",
         'nf() { local v=1; }\nnf'),
        ("three locals in one call",
         'nf() { local a=1 b=2 c=3; }\nnf'),
        ("1000 calls, one local each",
         'nf() { local v=1; }\ni=0\n'
         'while [ $i -lt 1000 ]; do nf; i=$((i+1)); done'),
        ("nested calls, locals at both depths",
         'b() { local y=2; }\na() { local x=1; b; }\na'),
        ("a local that shadows a real global",
         'v=outer\nnf() { local v=inner; }\nnf'),
    ):
        n = live(script)
        check("no-growth/%s" % name, n == base,
              "%d bytes vs a %d floor (+%d)" % (n, base, n - base))

    # Exiting from inside a function leaves saves unrestored, which is the
    # branch of the destructor that does the freeing rather than the
    # restoring -- and the one that segfaulted first time out.
    for name, script, want in (
        ("exit 0 holding one local",
         'nf() { local v=1; exit 0; }\nnf', 0),
        ("exit 3 holding two locals",
         'nf() { local v=1 w=2; exit 3; }\nnf', 3),
        ("exit 5 from a nested call, locals at both depths",
         'b() { local y=2; exit 5; }\na() { local x=1; b; }\na', 5),
    ):
        rc, out = run(script)
        check("teardown/%s" % name,
              rc == want and "Sanitizer" not in out
              and "Segmentation" not in out,
              "rc=%d (want %d) out=%r" % (rc, want, out[-200:]))

    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

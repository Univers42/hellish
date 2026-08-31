#!/usr/bin/env python3
"""Regression test: `VAR=x cmd` prefixes must not corrupt the heap.

Sourcing git's own contrib/completion/git-prompt.sh crashed the shell:

    malloc: failed assertion: free: unallocated block
    #5  ft_free ...
    #6  restore_one          src/execution/func_scope.c
    #7  restore_temp_assigns src/execution/func_scope2.c

Two places build a t_scope_save -- scope_save() for `local`, and
save_and_apply_one() for a temporary NAME=val prefix -- and each filled it
field by field. Adding attr_kind/attr_target (so `local -n` could unwind)
updated one of them. The other kept pushing an uninitialised pointer, which
restore_one then freed.

Two properties of that bug are worth remembering, because both made it
invisible:

  * it only appeared in an OPTIMISED build. Under ASan the stale stack slot
    happened to hold something benign and the debug binary ran clean, so the
    suite that runs ASan by default could never have caught it;
  * it needed enough allocation churn to matter. No single-line case
    reproduced it; it took a real 21KB script defining five functions with
    temp-assign prefixes throughout.

So this test does churn on purpose rather than asserting one clever case, and
`make pty-test`/CI should keep running it against a RELEASE build too.

Usage: python3 temp_assign_scope_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(script, timeout=90):
    return subprocess.run([SHELL, "-c", script], capture_output=True,
                          text=True, env=ENV, timeout=timeout)


def clean(name, script, want=None):
    """A crash shows up as a negative returncode (signal) or 134/139."""
    p = run(script)
    crashed = p.returncode < 0 or p.returncode in (134, 139)
    check(name, not crashed,
          "exited %d%s -- %r" % (p.returncode,
                                 " (SIGNAL)" if crashed else "",
                                 (p.stdout + p.stderr).strip()[:160]))
    if want is not None and not crashed:
        check(name + ": output", p.stdout.strip() == want,
              "got %r want %r" % (p.stdout.strip(), want))
    return p


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    clean("a temp assign restores cleanly",
          'f() { echo "$V"; }; V=1 f; V=2 f; echo done', "1\n2\ndone")

    # The shape that crashed: many prefixes, so the saves vec grows and is
    # rebuilt repeatedly.
    clean("500 temp assigns in a row",
          'f() { :; }; i=0; while [ $i -lt 500 ]; do '
          'A=$i B=$i C=$i f; i=$((i+1)); done; echo ok', "ok")

    # A prefix on a variable that HAS an attribute must restore the
    # attribute, not clear it -- that is what the new fields are for.
    clean("a temp assign preserves a nameref attribute",
          'v=TARGET; declare -n r=v; X=1 true; echo "$r"', "TARGET")

    # local and temp assigns interleaved, which is what mixes the two
    # construction sites in one run.
    clean("local and temp assigns interleave",
          'v=A; g() { local -n r=v; echo "$r"; }; '
          'X=1 g; Y=2 g; g; echo end', "A\nA\nA\nend")

    # A prefix on a variable that did not exist must leave it unset.
    clean("a temp assign on an unset name leaves it unset",
          'f() { :; }; NEW=1 f; echo "[${NEW-unset}]"', "[unset]")

    # ...and one that did exist must put the old value back.
    clean("a temp assign restores a previous value",
          'V=old; f() { echo "$V"; }; V=new f; echo "$V"', "new\nold")

    # Churn: functions defined, called with prefixes, redefined, unset --
    # the mix that a real plugin performs while loading.
    clean("define/call/redefine/unset churn under prefixes",
          'i=0; while [ $i -lt 200 ]; do '
          'eval "fn$i() { local q=1; echo -n \'\'; }"; '
          'P=$i Q=$i fn$i; unset -f fn$i; i=$((i+1)); done; echo survived',
          "survived")

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

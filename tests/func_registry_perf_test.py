#!/usr/bin/env python3
"""Regression test: the function registry must not go quadratic again.

func_lookup() was a linear scan over state->functions, and store_function()
calls it before every insert -- so DEFINING n functions was O(n^2) and
CALLING one was O(n). Measured against `bash --norc`, 2000 calls of a single
function with N other functions defined:

        N          hellish       bash     ratio
        0           3.6 ms      6.8 ms    0.5x   <- hellish TWICE AS FAST
      500          20.1 ms      7.1 ms    2.8x
     2000          74.3 ms     10.2 ms    7.3x
     5000         208.0 ms     13.2 ms   15.8x

hellish's dispatch is genuinely faster than bash's -- the scan is what threw
that away, and only once configs got big. A plugin ecosystem IS thousands of
functions, so this had to be fixed before rc.d/plugins ship, or the loader
would have been blamed for the registry.

WHAT THIS ASSERTS. Not wall-clock -- that is a flaky test on shared CI. It
asserts SCALING: quadrupling the number of defined functions must not
quadruple the cost of CALLING one. A hash lookup is flat; a linear scan is
not.

The call cost is ISOLATED by timing two scripts per size -- definitions
only, and definitions plus CALLS calls -- and subtracting. Timing one script
that does both conflates the O(n^2) definition cost with the O(n) call cost
and washes the signal out; an early draft of this test did exactly that and
PASSED against the linear scan it was written to catch.

Usage: python3 func_registry_perf_test.py [/path/to/hellish]
"""
import os
import statistics
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
SMALL, BIG, CALLS = 500, 2000, 2000
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def script(tmp, n, calls):
    p = os.path.join(tmp, "f%d_%d.hsh" % (n, calls))
    with open(p, "w") as f:
        for i in range(n):
            f.write("pad%d() { :; }\n" % i)
        f.write("target() { :; }\n")
        if calls:
            f.write("i=0\nwhile [ $i -lt %d ]; do target; i=$((i+1)); done\n"
                    % calls)
    return p


def call_cost(tmp, n):
    """Seconds spent on CALLS calls with n functions defined, with the
    definition cost subtracted out."""
    return max(timed(script(tmp, n, CALLS)) - timed(script(tmp, n, 0)), 1e-6)


def timed(path, reps=5):
    """Median wall time. Median, not mean: one scheduler hiccup on a loaded
    machine must not decide the verdict."""
    ts = []
    for _ in range(reps):
        t = time.perf_counter()
        subprocess.run([SHELL, "-c", ". %s; exit 0" % path],
                       capture_output=True, env=ENV, timeout=180)
        ts.append(time.perf_counter() - t)
    return statistics.median(ts)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    tmp = tempfile.mkdtemp()

    # Correctness first: an index is worthless if it returns the wrong body.
    p = subprocess.run(
        [SHELL, "-c",
         'a() { echo A; }; b() { echo B; }; a; b; '
         'a() { echo A2; }; a; unset -f b; b 2>/dev/null || echo NOB; a'],
        capture_output=True, text=True, env=ENV, timeout=30)
    check("lookup/redefine/unset stay correct",
          p.stdout.split() == ["A", "B", "A2", "NOB", "A2"],
          "got %r" % (p.stdout.split(),))

    # unset compacts the vec, which shifts every later slot. If the index is
    # not rebuilt, a later function resolves to the WRONG body -- silent and
    # nasty, so it gets its own check.
    p = subprocess.run(
        [SHELL, "-c",
         'f1() { echo one; }; f2() { echo two; }; f3() { echo three; }; '
         'unset -f f1; f2; f3'],
        capture_output=True, text=True, env=ENV, timeout=30)
    check("unset does not misalign the remaining functions",
          p.stdout.split() == ["two", "three"], "got %r" % (p.stdout.split(),))

    small = call_cost(tmp, SMALL)
    big = call_cost(tmp, BIG)
    ratio = big / small
    print("     %d calls with %d fns: %6.1f ms   with %d fns: %6.1f ms"
          "   ratio %.2fx" % (CALLS, SMALL, small * 1000, BIG, big * 1000,
                              ratio))
    # A flat lookup lands near 1.0; a scan over 4x the table lands near 4.0.
    # 2.0 sits in the empty gap between them, so noise cannot reach it and a
    # real regression cannot hide under it.
    check("calling a function does not slow down as the registry grows",
          ratio < 2.0,
          "%dx more functions made each call %.2fx more expensive "
          "(want <2x; a flat lookup is ~1x, the linear scan was ~4x)"
          % (BIG // SMALL, ratio))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

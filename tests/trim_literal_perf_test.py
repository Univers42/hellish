#!/usr/bin/env python3
"""Regression test: a literal ${v%pat} / ${v#pat} must not scan every position.

The idiom that peels characters off a string one at a time,

    while [ -n "$s" ]; do c=${s%"${s#?}"}; s=${s#?}; done

is everywhere shell prompt code measures or strips text. Its pattern is a
QUOTED copy of the rest of the string -- a literal, which can only match in
one place -- but trim_suffix_shortest ran the full matcher at every
position to find it, so each step cost O(n^2) and the loop O(n^3):

        chars      hellish      bash
          300       112 ms     20 ms
         1000      2903 ms    374 ms

A literal pattern is now answered with one comparison (trim_literal).

WHAT THIS ASSERTS. Scaling, not wall-clock: quadrupling the string may
multiply the loop's cost by about 16 (every shell copies the string on
each step, bash included), never by the ~64 a cubic loop costs. The loop
cost is isolated by subtracting a run that only builds the string. When
bash is present, hellish must also not be slower than it on the big case.

Usage: python3 trim_literal_perf_test.py [/path/to/hellish]
"""
import os
import shutil
import statistics
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
BASH = shutil.which("bash")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0", LC_ALL="C")
SMALL, BIG = 200, 800
BUILD = "s=$(printf '%%0%dd' 0); "
LOOP = 'while [ -n "$s" ]; do c="${s%"${s#?}"}"; s="${s#?}"; done'
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def timed(argv, cmd, reps=3):
    """Median wall time: one scheduler hiccup must not decide a verdict."""
    ts = []
    for _ in range(reps):
        t = time.perf_counter()
        subprocess.run(argv + ["-c", cmd], env=ENV, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        ts.append(time.perf_counter() - t)
    return statistics.median(ts)


def loop_cost(argv, n):
    build = BUILD % n
    return max(timed(argv, build + LOOP) - timed(argv, build + ":"), 1e-4)


def main():
    small = loop_cost([SHELL, "--norc"], SMALL)
    big = loop_cost([SHELL, "--norc"], BIG)
    ratio = big / small
    print("     hellish: %d chars %.1f ms, %d chars %.1f ms  (x%.1f)"
          % (SMALL, small * 1e3, BIG, big * 1e3, ratio))
    check("4x the characters costs at most ~32x, not the cubic ~64x",
          ratio < 32, "x%.1f" % ratio)
    if BASH:
        ref = loop_cost([BASH, "--norc"], BIG)
        print("     bash:    %d chars %.1f ms" % (BIG, ref * 1e3))
        check("no slower than bash on %d characters" % BIG,
              big <= ref * 1.5 + 0.05, "%.1f ms vs %.1f ms"
              % (big * 1e3, ref * 1e3))
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

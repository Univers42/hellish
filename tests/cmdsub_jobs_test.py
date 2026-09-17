#!/usr/bin/env python3
"""`$(jobs)` lists what bash's does, and leaves the job table alone.

Prompt frameworks count background jobs with `$(jobs)` before every
prompt. In hellish that forked, and the forked copy printed a stale
table: a job that had already finished still read "Running", because the
child cannot reap its parent's children. It now runs in-process (the
cmdsub fast path) in a read-only mode, which is also what bash's forked
`jobs` amounts to:

  * jobs still alive are listed, finished ones are not;
  * nothing is retired -- the shell's own `jobs` afterwards still reports
    the finished job as Done, exactly once;
  * `jobs -p` and `jobs -l` follow the same rule.

Each case runs under hellish and bash with the same generous sleeps (a
loaded machine must not flip a verdict), PIDs masked.

Usage: python3 cmdsub_jobs_test.py [/path/to/hellish]
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
# The pinned oracle when it is built, as tests/tester prefers it. The
# `jobs` format is one of the things bash changed between releases: 5.1
# pads the status column to 24, 5.3 to 27, and 5.3 announces a finished
# job that 5.1 swallows -- so against the system bash (5.1 on Ubuntu 22.04)
# a correct hellish reads as three failures. CI puts the oracle first on
# PATH; a developer's machine does not.
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
BASH = ORACLE if os.path.exists(ORACLE) else shutil.which("bash")
FAILS = []

# job 1 finishes at once, job 2 outlives the whole case
SETUP = "sleep 0.05 & sleep 5 & sleep 1; "
CASES = [
    ("running only", SETUP + 'x=$(jobs); echo "[$x]"; kill %2; wait'),
    ("-p lists live pids", SETUP + 'x=$(jobs -p); echo "[$x]"; kill %2; wait'),
    ("-l lists live jobs", SETUP + 'x=$(jobs -l); echo "[$x]"; kill %2; wait'),
    ("the shell still reports the finished job",
     SETUP + 'x=$(jobs); jobs; echo ---; jobs; kill %2; wait'),
    ("twice in a row", SETUP + 'a=$(jobs); b=$(jobs); [ "$a" = "$b" ] '
     '&& echo same; kill %2; wait'),
    ("no jobs", 'x=$(jobs); echo "[$x]"'),
]


def run(argv, cmd):
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", LC_ALL="C")
    p = subprocess.run(argv + ["-c", cmd], capture_output=True, timeout=30,
                       env=env, stdin=subprocess.DEVNULL)
    out = p.stdout.decode("utf-8", "replace")
    return re.sub(r"\b\d{2,}\b", "PID", out), p.returncode


def main():
    if not BASH:
        print("skip: no bash to compare with")
        return 0
    for name, cmd in CASES:
        got = run([SHELL, "--norc"], cmd)
        want = run([BASH], cmd)
        ok = got == want
        print(("ok   " if ok else "FAIL ") + name
              + ("" if ok else "\n       hellish %r\n       bash    %r"
                 % (got, want)))
        if not ok:
            FAILS.append(name)
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

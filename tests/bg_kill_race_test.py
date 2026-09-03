#!/usr/bin/env python3
"""`cmd & kill %1` must never say "No such process" -- the arm64 flake.

    sleep 0.3 & kill %1; wait; jobs

failed the golden suite on the arm64 runner every other push, and on an
x86 box under load one run in fifteen: `kill %1` answered
"kill: (PID): No such process", the sleep ran to completion, and `jobs`
had nothing to report where bash prints "Terminated". strace showed
kill(-PID, SIGTERM) = ESRCH: the shell signals the job's process GROUP,
and only the child created that group (setpgid(0, 0) after fork) -- so
whenever the parent reached `kill` before the child was scheduled, the
group did not exist yet. The parent now calls setpgid(pid, pid) too,
as it already did for foreground jobs.

A race is only caught by running it many times under contention, so
this test starts a few busy loops, runs the case 200 times, and requires
zero "No such process" and a successful kill every time. (Whether `jobs`
then says "Terminated" after `wait` reaped the job is timing-dependent
in bash itself, so it is not asserted.) Before the fix that is a
virtual certainty to trip (1 in 15 per run under load); after it, none
in thousands.

Usage: python3 bg_kill_race_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
CASE = 'sleep 0.05 & kill %1; echo "k=$?"; wait 2>/dev/null; echo "done=$?"'
RUNS = 200


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")
    # contention: a few spinners for the whole run
    load = [subprocess.Popen(["sh", "-c", "while :; do :; done"],
                             stdout=subprocess.DEVNULL) for _ in range(4)]
    esrch = 0
    refused = 0
    t0 = time.time()
    try:
        for _ in range(RUNS):
            p = subprocess.run([SHELL, "-c", CASE], env=env, capture_output=True,
                               text=True, timeout=20)
            if "No such process" in p.stderr:
                esrch += 1
            if "k=0" not in p.stdout:
                refused += 1
    finally:
        for l in load:
            l.kill()
    check("kill %%1 never misses a job it just started (%d runs, %.0fs)"
          % (RUNS, time.time() - t0), esrch == 0,
          "%d runs answered 'No such process'" % esrch)
    check("...and kill %1 reports success every time", refused == 0,
          "%d runs had kill fail" % refused)
    print("\n%s" % ("ALL PASSED" if not FAILS else "%d FAILED: %s"
                    % (len(FAILS), ", ".join(FAILS))))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

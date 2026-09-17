#!/usr/bin/env python3
"""Performance gate for the interactive front end, priced in SYSCALLS.

A stopwatch in CI grades the machine, not the shell (RELEASE.md: "stop CI
grading a stopwatch"). Syscalls do not flake: the work a prompt does is the
same on a loaded machine as on an idle one, and every regression this file
exists to catch shows up as work --

  a fork coming back        clone/pipe2/wait4 per prompt, the cost the
                            in-process reader was written to remove
  a polling idle loop       an idle prompt must cost NOTHING: the shell
                            blocks in one wait until a key or a signal
  a redraw on every signal  a tab switch (same-size SIGWINCH) and a
                            row-count change must not write one byte
  per-keystroke bloat       an extra wait or measurement per key

Budgets sit just above what the shell does today, so they pin the wins
rather than leave room to lose them. Measured on this machine, per bare
Enter: in-process 48 syscalls (10 of them writes, 14 ioctls, no clone),
forked 68 (one clone, two pipe2, one wait4); per keystroke 7 both ways;
idle 0; bash 5.3 in the same harness: 43 per Enter, 4 per keystroke.

Phases are cut by wall clock against strace -ttt, and -f follows the forked
reader's child so its work counts against it.

Usage: python3 frontend_budget_test.py /path/to/hellish
"""
import collections
import fcntl
import os
import pty
import re
import select
import shutil
import signal
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
STRACE = shutil.which("strace")
FAILS = []
MARK = b"#> "
ENTERS = 20
KEYS = 40
TRACE_LINE = re.compile(r"^(\d+)\s+(\d+\.\d+)\s+([a-z_0-9]+)")

# name -> (per Enter, per key, idle total, writes per resize phase)
BUDGET = {
    "in-process": {"enter": 55, "enter_write": 12, "enter_ioctl": 16,
                   "key": 9, "key_write": 3},
    "forked": {"enter": 85, "enter_write": 13, "enter_ioctl": 16,
               "key": 9, "key_write": 3},
}
FORKING = ("clone", "clone3", "fork", "vfork", "pipe2", "pipe", "wait4",
           "execve")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Traced:
    def __init__(self, home, trace, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "PS1": "top\\nIN#> ", "RPROMPT": "12:34",
            "HISTFILE": "/dev/null", "ASAN_OPTIONS": "detect_leaks=0",
        }
        env.update(extra)
        self.buf = bytearray()
        self.phases = []
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            fcntl.ioctl(0, termios.TIOCSWINSZ,
                        struct.pack("HHHH", 24, 100, 0, 0))
            os.chdir(home)
            os.execve(STRACE, [STRACE, "-f", "-qq", "-ttt", "-o", trace,
                               "--", SHELL, "--norc"], env)
            os._exit(127)
        self.until(lambda: MARK in self.buf, 20.0)
        self.settle()

    def pump(self, secs):
        r, _, _ = select.select([self.fd], [], [], secs)
        if not r:
            return False
        try:
            d = os.read(self.fd, 65536)
        except OSError:
            return False
        if not d:
            return False
        self.buf.extend(d)
        return True

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def settle(self, quiet=0.3, cap=3.0):
        end = time.monotonic() + cap
        while time.monotonic() < end and self.pump(quiet):
            pass

    def phase(self, name, fn):
        t0 = time.time()
        fn()
        self.phases.append((name, t0, time.time()))

    def enters(self):
        n = self.buf.count(MARK)
        os.write(self.fd, b"\r" * ENTERS)
        self.until(lambda: self.buf.count(MARK) >= n + ENTERS, 30.0)
        self.settle()

    def typing(self):
        for _ in range(KEYS):
            os.write(self.fd, b"x")
            self.pump(0.03)
        self.settle()
        os.write(self.fd, b"\x15")
        self.settle()

    def idle(self):
        time.sleep(2.0)

    def winch_same(self):
        pgrp = os.tcgetpgrp(self.fd)
        for _ in range(5):
            os.killpg(pgrp, signal.SIGWINCH)
            self.settle(0.1, 0.4)
        self.settle()

    def rows_only(self):
        for rows in (23, 24, 23, 24):
            fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                        struct.pack("HHHH", rows, 100, 0, 0))
            self.settle(0.1, 0.4)
        self.settle()

    def close(self):
        try:
            os.write(self.fd, b"\x05\x15exit\r")
            self.until(lambda: False, 1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def counts(lines, t0, t1):
    c = collections.Counter()
    for line in lines:
        m = TRACE_LINE.match(line)
        if m and t0 <= float(m.group(2)) <= t1 and "resumed>" not in line:
            c[m.group(3)] += 1
    return c


def cell(label, extra, base):
    trace = os.path.join(base, "trace-" + label)
    home = tempfile.mkdtemp(dir=base)
    s = Traced(home, trace, extra)
    s.phase("enter", s.enters)
    s.phase("idle", s.idle)
    s.phase("key", s.typing)
    s.phase("winch", s.winch_same)
    s.phase("rows", s.rows_only)
    s.close()
    time.sleep(0.5)
    lines = open(trace).read().splitlines()
    by = {name: counts(lines, t0, t1) for name, t0, t1 in s.phases}
    b = BUDGET[label]

    per = sum(by["enter"].values()) / float(ENTERS)
    check("%s: a bare Enter costs at most %d syscalls" % (label, b["enter"]),
          per <= b["enter"], "%.1f per Enter: %s" % (
              per, dict(by["enter"].most_common(6))))
    for name, key in (("write", "enter_write"), ("ioctl", "enter_ioctl")):
        n = by["enter"][name] / float(ENTERS)
        check("%s: a bare Enter does at most %d %ss" % (label, b[key], name),
              n <= b[key], "%.1f per Enter" % n)
    forks = sum(by["enter"][k] for k in FORKING)
    if label == "in-process":
        check("%s: a bare Enter creates no process" % label, forks == 0,
              "%s" % {k: by["enter"][k] for k in FORKING if by["enter"][k]})
    else:
        spawns = sum(by["enter"][k] for k in ("clone", "clone3", "fork",
                                              "vfork"))
        check("%s: a bare Enter forks exactly one child" % label,
              spawns == ENTERS, "%d children for %d Enters (%s)" % (
                  spawns, ENTERS, dict((k, by["enter"][k]) for k in FORKING
                                       if by["enter"][k])))

    per = sum(by["key"].values()) / float(KEYS)
    check("%s: a keystroke costs at most %d syscalls" % (label, b["key"]),
          per <= b["key"], "%.1f per key: %s" % (
              per, dict(by["key"].most_common(6))))
    n = by["key"]["write"] / float(KEYS)
    check("%s: a keystroke does at most %d writes" % (label, b["key_write"]),
          n <= b["key_write"], "%.1f per key" % n)

    check("%s: an idle prompt costs nothing" % label,
          sum(by["idle"].values()) == 0,
          "%d syscalls while idle: %s" % (sum(by["idle"].values()),
                                          dict(by["idle"].most_common(5))))
    check("%s: a same-size SIGWINCH writes nothing" % label,
          by["winch"]["write"] == 0, "%d writes" % by["winch"]["write"])
    check("%s: a row-count change writes nothing" % label,
          by["rows"]["write"] == 0, "%d writes" % by["rows"]["write"])


def main():
    if not STRACE:
        print("skip: strace not installed (apt-get install strace)")
        sys.exit(0)
    base = tempfile.mkdtemp(prefix="hellish_frontend_budget_")
    try:
        cell("in-process", {}, base)
        cell("forked", {"HELLISH_RL_FORK": "1"}, base)
    finally:
        shutil.rmtree(base, ignore_errors=True)
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

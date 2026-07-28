#!/usr/bin/env python3
"""Regression test: the git dirty check must never block the prompt.

Guards the async dirty check in prompt_git3.c. The original run_git_check
forked `git status --porcelain -uno` and then blocked in read()+waitpid()
until the whole scan finished: cd into a repo where git status takes
seconds froze the first prompt for the full scan (the "cd is slow" bug),
and again on every 3s TTL refresh. The check now waits a perception-bound
slice for a freshly entered repo and otherwise finishes in the background,
harvested by a later render — bash-instant prompts, star at most one
render late.

Checks, in a real pty, default two-row prompt (PS1 unset):
  1. With a `git` shim sleeping 2s, cd into a repo prompts fast
     (well under the shim delay) instead of after the scan.
  2. A later prompt in that repo is also fast, and the star the
     background scan found has arrived by then (async harvest).
  3. With real git, cd into a repo with tracked modifications still
     shows the amber star (bounded wait keeps normal repos exact).
  4. After the TTL expires there, the refresh keeps the prompt fast
     and keeps showing the last known star while it re-checks.
  5. A clean repo shows its branch and no star.

Usage: python3 git_prompt_stall_test.py /path/to/hellish
"""
import os
import pty
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import fcntl
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
GIT = shutil.which("git") or "/usr/bin/git"
BOX = "╭".encode()  # one per default-prompt render
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home, cwd, path):
        env = {
            "HOME": home, "PATH": path,
            "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.raw = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd)
            os.environ.clear()
            os.environ.update(env)
            os.execvp(SHELL, [SHELL])
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 24, 100, 0, 0))

    def drain(self, t=0.35):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.08)
            if r:
                try:
                    self.raw += os.read(self.fd, 65536)
                except OSError:
                    return

    def prompt_latency(self, data, timeout=10.0):
        """Send data; seconds until the NEXT prompt render, or None."""
        n0 = self.raw.count(BOX)
        t0 = time.monotonic()
        os.write(self.fd, data)
        while time.monotonic() - t0 < timeout:
            r, _, _ = select.select([self.fd], [], [], 0.02)
            if not r:
                continue
            try:
                self.raw += os.read(self.fd, 65536)
            except OSError:
                return None
            if self.raw.count(BOX) > n0:
                return time.monotonic() - t0
        return None

    def close(self):
        try:
            os.write(self.fd, b"\x04")
            self.drain(0.3)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def make_repo(base, name, dirty):
    d = os.path.join(base, name)
    os.makedirs(d)
    env = {"PATH": os.environ["PATH"], "HOME": base,
           "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null"}
    run = lambda *a: subprocess.run(
        [GIT, "-C", d] + list(a), env=env, check=True,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    run("init", "-q", "-b", "main")
    open(os.path.join(d, "f.txt"), "w").write("x\n")
    run("add", "f.txt")
    run("-c", "user.email=t@t", "-c", "user.name=t", "commit", "-qm", "i")
    if dirty:
        open(os.path.join(d, "f.txt"), "a").write("y\n")
    return d


def main():
    base = tempfile.mkdtemp(prefix="hellish_gitprompt_")
    work = os.path.join(base, "work")
    os.makedirs(work)
    shim = os.path.join(base, "gitshim")
    os.makedirs(shim)
    with open(os.path.join(shim, "git"), "w") as f:
        f.write("#!/bin/sh\nsleep 2\nexec %s \"$@\"\n" % GIT)
    os.chmod(os.path.join(shim, "git"), 0o755)
    slow = make_repo(base, "slow_repo", dirty=True)
    dirty = make_repo(base, "dirty_repo", dirty=True)
    clean = make_repo(base, "clean_repo", dirty=False)

    # 1+2: a repo where git status takes 2s must not delay the prompt;
    # the answer it eventually finds shows up on a later render
    s = Session(base, work, shim + ":" + os.environ["PATH"])
    s.prompt_latency(b"")
    s.drain(0.4)
    lat = s.prompt_latency(("cd %s\n" % slow).encode())
    check("cd into slow-git repo prompts fast",
          lat is not None and lat < 1.0, "latency=%s" % lat)
    s.drain(2.6)  # background scan finishes during this idle
    n0 = len(s.raw)
    lat = s.prompt_latency(b"\n")
    check("later prompt in slow repo stays fast",
          lat is not None and lat < 1.0, "latency=%s" % lat)
    s.drain(0.4)
    check("slow repo star arrives async", b"*" in s.raw[n0:])
    s.close()

    # 3: normal-speed repo with tracked modifications still gets the star
    s = Session(base, work, os.environ["PATH"])
    s.prompt_latency(b"")
    s.drain(0.4)
    n0 = len(s.raw)
    lat = s.prompt_latency(("cd %s\n" % dirty).encode())
    s.drain(0.4)
    win = s.raw[n0:]
    check("dirty repo shows branch", b"on\x1b[0m" in win and b"main" in win)
    check("dirty repo shows star", b"*" in win)

    # 4: TTL refresh re-spawns with a zero budget: fast prompt, stale
    # star kept while the fresh answer is pending
    s.drain(3.2)  # let the 3s TTL expire
    n0 = len(s.raw)
    lat = s.prompt_latency(b"\n")
    check("TTL refresh never blocks the prompt",
          lat is not None and lat < 1.0, "latency=%s" % lat)
    s.drain(0.4)
    check("TTL refresh keeps last known star", b"*" in s.raw[n0:])

    # 5: clean repo shows the branch and no star
    n0 = len(s.raw)
    s.prompt_latency(("cd %s\n" % clean).encode())
    s.drain(0.4)
    win = s.raw[n0:]
    check("clean repo shows branch", b"on\x1b[0m" in win and b"main" in win)
    check("clean repo shows no star", b"*" not in win)
    s.close()

    shutil.rmtree(base, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

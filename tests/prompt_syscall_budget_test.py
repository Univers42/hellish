#!/usr/bin/env python3
"""Regression gate: what one bare Enter is allowed to cost, in syscalls.

The latency gate (prompt_latency_test.py) measures time, and time is noisy.
This one counts, and counts are exact: strace the interactive shell while
the user presses Enter on an empty line N times, then read the process
creations and file opens each prompt caused. A regression that forks a
`git status` per prompt, re-reads the update state file every cycle, or
re-parses ~/.inputrc in a fresh readline child shows up here as an integer,
on any machine, with no bound to tune.

Budget, per bare Enter, inside a git repo, with the fixture rc loaded
(tests/fixtures/frontend.hellishrc -- coloured PS1 with the \\g badge,
RPROMPT, three hooks):

  execve            0   nothing external runs when the user runs nothing
  clone/fork        <= MAX_CLONES   the readline child (0 once readline runs
                                    in-process); never a git status
  openat            <= MAX_OPENS    .git/HEAD for the branch name, and the
                                    update state at most once per TTL

When this file first landed a bare Enter cost 2 clones + 1 execve (the git
check) + a handful of opens (terminfo, inputrc, update state), every prompt.

A second run swaps in the shape of a prompt FRAMEWORK: PS1 built by a
PROMPT_COMMAND function that reads the repository through vcs_info's
HELLISH_GIT_* variables and, like every real rc, is full of `>/dev/null`.
Each of those redirections used to count as "the working tree may have
changed", so the git answer was thrown away and a `git status` spawned on
every bare Enter -- 20 execve for 20 Enters, from a hook that ran nothing.
Its open budget is three higher: the hook's own three /dev/null opens.
Its PS1 has \\u, which read /etc/passwd on every prompt when $USER was
unset (as it is here); the name is looked up once now.

Skips (exit 0, says so) when strace is not installed; CI installs it.

Usage: python3 prompt_syscall_budget_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURE = os.path.join(HERE, "fixtures", "frontend.hellishrc")
STRACE = shutil.which("strace")
GIT = shutil.which("git")
ENTERS = 20
MAX_CLONES = 1
MAX_OPENS = 2
MARK = "❯".encode()
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def make_repo(base):
    d = os.path.join(base, "repo")
    os.makedirs(d)
    if not GIT:
        return d
    env = {"PATH": os.environ["PATH"], "HOME": base,
           "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null"}
    run = lambda *a: subprocess.run(
        [GIT, "-C", d] + list(a), env=env, check=True,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    run("init", "-q", "-b", "main")
    open(os.path.join(d, "f.txt"), "w").write("x\n")
    run("add", "f.txt")
    run("-c", "user.email=t@t", "-c", "user.name=t", "commit", "-qm", "i")
    open(os.path.join(d, "f.txt"), "a").write("y\n")
    return d


class Traced:
    def __init__(self, home, cwd, trace):
        env = {
            "HOME": home, "XDG_CONFIG_HOME": os.path.join(home, ".config"),
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        }
        self.buf = bytearray()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd)
            os.environ.clear()
            os.environ.update(env)
            os.execvp(STRACE, [STRACE, "-f", "-qq", "-o", trace,
                               "-e", "trace=execve,clone,clone3,fork,vfork,openat",
                               "--", SHELL])
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 24, 100, 0, 0))

    def drain(self, cap=8.0, quiet=0.5):
        last = time.monotonic()
        end = last + cap
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    return
                if not d:
                    return
                self.buf.extend(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                return

    def hold_enter(self, n):
        """n bare Enters as fast as the terminal will take them, waited
        until n more prompts have been drawn.

        A BURST, not n paced presses, and that matters twice over. It is
        the reported scenario -- a key held down. And the prompt's git
        cache has a 3-second TTL, so pacing the presses half a second
        apart lets the TTL expire mid-run and measures the clock instead
        of the cache: an early draft did exactly that and reported MORE
        git spawns after the fix that removed them."""
        n0 = self.buf.count(MARK)
        os.write(self.fd, b"\r" * n)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 20.0 and self.buf.count(MARK) < n0 + n:
            self.drain(0.5, quiet=0.1)
        self.drain(1.5, quiet=0.4)
        return self.buf.count(MARK) - n0

    def close(self):
        try:
            os.write(self.fd, b"exit\r")
            self.drain(1.5)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def count(lines, pattern):
    rx = re.compile(pattern)
    return sum(1 for l in lines if rx.search(l))


HOOK_RC = r"""
PS1='\u \w ❯ '
hk_prompt() {
    vcs_info >/dev/null 2>&1
    : >/dev/null 2>/dev/null
    HK_LABEL="${HELLISH_GIT_BRANCH}${HELLISH_GIT_AHEAD}${HELLISH_GIT_STASH}"
}
PROMPT_COMMAND=hk_prompt
"""


def run_case(label, rc_text, max_opens, max_execs=0, max_clones=MAX_CLONES):
    base = tempfile.mkdtemp(prefix="hellish_syscalls_")
    home = os.path.join(base, "home")
    os.makedirs(home)
    with open(os.path.join(home, ".hellishrc"), "w") as f:
        f.write(rc_text)
    cwd = make_repo(base)
    trace = os.path.join(base, "trace")

    s = Traced(home, cwd, trace)
    s.drain(10.0)
    check("%s: prompt appeared under strace" % label, s.buf.count(MARK) >= 1,
          repr(bytes(s.buf[-300:])))
    s.drain(3.0)  # let startup-time background work (git scan) finish
    with open(trace, errors="replace") as f:
        mark = len(f.readlines())
    drawn = s.hold_enter(ENTERS)
    check("%s: all %d prompts were drawn" % (label, ENTERS), drawn >= ENTERS,
          "only %d" % drawn)
    s.drain(3.0)
    with open(trace, errors="replace") as f:
        lines = f.readlines()[mark:]
    s.close()

    execs = count(lines, r"\bexecve\(")
    clones = count(lines, r"\b(clone3?|fork|vfork)\(")
    opens = count(lines, r"\bopenat\(")
    per = lambda n: n / float(ENTERS)
    print("     %s, %d bare Enters: execve=%d clone=%d openat=%d  "
          "-> per Enter: %.2f / %.2f / %.2f"
          % (label, ENTERS, execs, clones, opens,
             per(execs), per(clones), per(opens)))
    if execs:
        seen = [l.strip() for l in lines if "execve(" in l][:3]
        print("     first execve lines:\n       " + "\n       ".join(seen))
    if opens:
        paths = re.findall(r'openat\([^,]*, "([^"]*)"', "".join(lines))
        top = {}
        for p in paths:
            top[p] = top.get(p, 0) + 1
        print("     opens: " + ", ".join("%s x%d" % kv for kv in
                                          sorted(top.items(), key=lambda kv: -kv[1])[:6]))

    check("%s: a bare Enter executes at most %d program" % (label, max_execs),
          per(execs) <= max_execs, "%d execve for %d Enters" % (execs, ENTERS))
    check("%s: a bare Enter creates at most %d process" % (label, max_clones),
          per(clones) <= max_clones, "%.2f clones per Enter" % per(clones))
    check("%s: a bare Enter opens at most %d files" % (label, max_opens),
          per(opens) <= max_opens, "%.2f opens per Enter" % per(opens))
    shutil.rmtree(base, ignore_errors=True)


def main():
    if not STRACE:
        print("skip: strace not installed (apt-get install strace)")
        sys.exit(0)
    with open(FIXTURE) as f:
        run_case("fixture rc", f.read(), MAX_OPENS)
    run_case("framework hook", HOOK_RC, MAX_OPENS + 3)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

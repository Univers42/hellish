#!/usr/bin/env python3
"""Minimal-environment robustness (issue #98).

A login shell genuinely meets near-empty environments: display managers,
`env -i` wrappers, containers, sshd with a stripped config. hellish 2.8.3
segfaulted the moment `exit` was typed in an `env -i TERM=xterm` pty:
with no HOME, parse_history_file() returns before installing the
hist_cmds vector, leaving elem_size 0 -- vec_push then grows len while
storing nothing, and both the history dedup (strlen through a garbage
pointer) and the exit teardown (free of a wild pointer) walk off the
zero-stride buffer.

This sweeps the whole class, not just the reported crash:
  pty sessions with no HOME/PATH/USER/SHELL:
    1. bare `exit` as the first command      (the reported SEGV)
    2. commands + duplicate, then exit        (the dedup read path)
    3. `history` listing, then exit
    4. history -s / -c mutation, then exit
    5. cd + `echo ~` with no HOME, then exit  (must not crash; cd errors)
    6. HOME set to a nonexistent dir          (history file open fails)
  non-interactive, same env:
    7. hellish -c 'echo ok'
    8. piped stdin
    9. absolute-path exec with no PATH at all

Every session must exit 0 with its marker in the transcript, die on no
signal, and print no sanitizer report.

Usage: python3 empty_env_test.py [/path/to/hellish]
"""
import os
import pty
import select
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

BASE_ENV = {
    "TERM": "xterm",
    "HELLISH_BANNER": "0",
    "HELLISH_NO_ANIM": "1",
    "HELLISH_NO_UPDATE_CHECK": "1",
}


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def pty_session(cmds, env):
    """Run an interactive session under `env` only, sending each command
    in order and `exit` last. Returns (transcript, wait status)."""
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL, "--norc"], env)
        os._exit(127)
    time.sleep(0.6)
    for c in cmds + ["exit"]:
        os.write(fd, c.encode() + b"\r")
        time.sleep(0.3)
    out = b""
    deadline = time.time() + 8
    status = None
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.3)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
        else:
            wpid, st = os.waitpid(pid, os.WNOHANG)
            if wpid:
                status = st
                break
    try:
        os.close(fd)
    except OSError:
        pass
    if status is None:
        _, status = os.waitpid(pid, 0)
    return out.decode("utf-8", "replace"), status


def session_check(name, cmds, env=None):
    e = dict(BASE_ENV)
    if env:
        e.update(env)
    text, st = pty_session(cmds, e)
    clean = (os.WIFEXITED(st) and os.WEXITSTATUS(st) == 0
             and "AddressSanitizer" not in text
             and "Segmentation fault" not in text)
    detail = "status=%r tail=%r" % (st, text[-300:])
    check(name, clean, detail)
    return text


def main():
    session_check("bare exit, no HOME/PATH (the #98 SEGV)", [])
    t = session_check("dedup path: repeated command then exit",
                      ["echo m1", "echo m2", "echo m1", "echo DONE-2"])
    check("dedup session ran its commands", "DONE-2" in t, repr(t[-300:]))
    session_check("history listing with no HOME", ["history", "echo DONE-3"])
    session_check("history -s / -c with no HOME",
                  ["history -s injected", "history", "history -c",
                   "echo DONE-4"])
    t = session_check("cd and tilde with no HOME",
                      ["cd", "echo ~", "echo DONE-5"])
    check("cd-no-HOME session stayed alive", "DONE-5" in t, repr(t[-300:]))
    session_check("HOME points at a nonexistent dir",
                  ["echo DONE-6"], {"HOME": "/nonexistent-hellish-98"})

    p = subprocess.run([SHELL, "-c", "echo nc-ok"], env=dict(BASE_ENV),
                       capture_output=True, text=True, timeout=60)
    check("-c under env -i", p.returncode == 0
          and p.stdout.strip() == "nc-ok", repr((p.returncode, p.stdout)))
    p = subprocess.run([SHELL], input="echo pipe-ok\n", env=dict(BASE_ENV),
                       capture_output=True, text=True, timeout=60)
    check("piped stdin under env -i", p.returncode == 0
          and p.stdout.strip() == "pipe-ok", repr((p.returncode, p.stdout)))
    p = subprocess.run([SHELL, "-c", "/bin/echo abs-ok"], env=dict(BASE_ENV),
                       capture_output=True, text=True, timeout=60)
    check("absolute-path exec with no PATH", p.returncode == 0
          and p.stdout.strip() == "abs-ok", repr((p.returncode, p.stdout)))

    if FAILS:
        print("\n%d FAILED: %s" % (len(FAILS), ", ".join(FAILS)))
        sys.exit(1)
    print("\nall clear")


main()

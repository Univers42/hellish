#!/usr/bin/env python3
"""The HIST* variables, diffed against the pinned bash in a real pty.

HISTCONTROL, HISTIGNORE, HISTSIZE, HISTFILESIZE and HISTTIMEFORMAT appeared
in this tree only inside a comment. Nothing read them. The dedup rule was
hard-wired to bash's `ignoredups` whatever the user had asked for -- which
is a divergence in BOTH directions, because bash's default HISTCONTROL is
empty and keeps consecutive duplicates. So a configuration saying

    HISTCONTROL=ignoredups:ignorespace:erasedups
    HISTIGNORE='ls:cd:exit'
    HISTSIZE=20000

got the first word of the first line by accident and nothing else.

Each case runs the same rc and the same keystrokes through hellish and
through bash 5.3.9, and diffs the numbered `history` output. bash is the
definition of correct here, so the assertion is equality with it rather
than a hand-written expectation that could encode the same misreading
twice.

Two of these also pin behaviour that is NOT dedup:

  - `history` lists itself, because bash records a line when it reads it,
    not after it runs. The listing used to always stop one short.
  - a repeated multi-line command dedups, which it could not before: the
    check compared the RAW typed text against the STORED entry, and the
    stored entry is the joined one-liner, so the two were never equal for
    anything spanning more than a line.

Usage: python3 history_vars_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(shell, rc, cmds):
    home = tempfile.mkdtemp(prefix="hellish_histvars_")
    for name in (".hellishrc", ".bashrc"):
        with open(os.path.join(home, name), "w") as f:
            f.write(rc)
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "dumb", "LANG": "C.UTF-8", "PS1": "> ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    if shell.endswith("hellish"):
        argv = [shell]
    else:
        argv = [shell, "--noprofile", "--rcfile",
                os.path.join(home, ".bashrc")]
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(shell, argv)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    buf = bytearray()

    def drain(quiet=0.5, cap=8.0):
        last = time.monotonic()
        end = last + cap
        while time.monotonic() < end:
            r, _, _ = select.select([fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(fd, 65536)
                except OSError:
                    return
                if not d:
                    return
                buf.extend(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                return

    drain(1.2)
    for c in cmds:
        os.write(fd, (c + "\n").encode())
        drain(0.35)
    os.write(fd, b"exit\n")
    drain(0.5)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    shutil.rmtree(home, ignore_errors=True)
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]|[\x01\x02]", b"",
                  bytes(buf)).decode("utf-8", "replace")


def listing(txt):
    """The entries of a `history` dump, in order, without the `exit`."""
    out = []
    for line in txt.splitlines():
        m = re.match(r"\s*\d+\s+(.*)$", line.rstrip())
        if m and m.group(1) != "exit":
            out.append(m.group(1))
    return out


CASES = [
    ("HISTCONTROL unset keeps duplicates", "unset HISTCONTROL\n",
     ["echo a", "echo a", "echo b", "history"]),
    ("ignoredups", "HISTCONTROL=ignoredups\n",
     ["echo a", "echo a", "echo b", "history"]),
    ("ignorespace hides a space-led line", "HISTCONTROL=ignorespace\n",
     ["echo a", " echo secret", "echo b", "history"]),
    ("ignoreboth is the pair", "HISTCONTROL=ignoreboth\n",
     ["echo a", "echo a", " echo s", "echo b", "history"]),
    ("erasedups keeps only the newest copy", "HISTCONTROL=erasedups\n",
     ["echo a", "echo b", "echo a", "history"]),
    ("HISTIGNORE takes glob patterns",
     "HISTIGNORE='ls:cd*'\nunset HISTCONTROL\n",
     ["echo a", "ls", "cd /tmp", "echo b", "history"]),
    ("HISTIGNORE '&' means the previous line",
     "HISTIGNORE='&'\nunset HISTCONTROL\n",
     ["echo a", "echo a", "echo b", "history"]),
    ("HISTSIZE caps the live list", "HISTSIZE=3\nunset HISTCONTROL\n",
     ["echo 1", "echo 2", "echo 3", "echo 4", "history"]),
    ("history lists itself", "unset HISTCONTROL\n",
     ["echo a", "history"]),
    ("a repeated multi-line command dedups", "HISTCONTROL=ignoredups\n",
     ["for i in 1 2; do echo $i; done",
      "for i in 1 2; do echo $i; done", "history"]),
]


def main():
    if not os.path.exists(ORACLE):
        print("skip: no pinned oracle at %s -- run `make oracle`" % ORACLE)
        sys.exit(0)
    for name, rc, cmds in CASES:
        got = listing(run(SHELL, rc, cmds))
        want = listing(run(ORACLE, rc, cmds))
        check(name, got == want, "hellish=%s bash=%s" % (got, want))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

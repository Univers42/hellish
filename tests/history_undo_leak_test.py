#!/usr/bin/env python3
"""Regression gate: dropping a history entry frees what readline hung on it.

Field report, from a session with HISTCONTROL=erasedups: `theme next` started
printing a LeakSanitizer report per prompt, one per `$(...)` the theme ran --

    Direct leak of 32 byte(s) in 1 object(s) allocated from:
        #1 xmalloc (libreadline.so.8)
        #2 rl_add_undo
        #3 rl_insert_text

Edit a recalled line and arrow off it, and readline keeps the edit: the
entry's `data` becomes that line's undo list, so returning to it can still
undo. remove_history() and free_history_entry() hand the data back to the
CALLER, and clear_history() says in a comment that it "loses because we
cannot free the data". hellish ignored it in all four places it drops
entries. While readline ran in a forked child the leak died with the child;
once the line is read in the shell itself it is the shell's own heap, and
every subshell forked afterwards inherits and reports it. history_rl.c frees
the undo list (never the one readline is still editing, which a zle widget
running `history -d` could reach).

Each flow: seed history, recall an entry, EDIT it, arrow away (readline
stores the undo list on the entry), then make hellish drop that entry --
HISTCONTROL=erasedups, the HISTSIZE cap, `history -d`, `history -c` -- and
fork a child, whose exit runs LSan over the inherited heap. Control flows
(browsing without editing, editing without dropping) must stay quiet too,
so a failure means the leak and not a noisy baseline.

Needs an ASan build (`make all`); on any other binary it reports skip,
because SAFE=0's ft_malloc has no leak oracle for readline's libc heap.

Usage: python3 history_undo_leak_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []
EDIT = [b"\x1b[A", b"\x1b[A", b"X", b"\x1b[B", b"\x1b[B", b"\x15"]

# (label, keys, optional (command, text that must be gone afterwards))
FLOWS = [
    ("erasedups: the duplicate drops the edited entry",
     [b"HISTCONTROL=erasedups\r", b"echo one\r", b"echo two\r"]
     + EDIT + [b"echo one\r"], None),
    ("HISTSIZE: the cap drops the edited entry",
     [b"HISTSIZE=3\r", b"echo one\r", b"echo two\r"]
     + EDIT + [b"echo three\r", b"echo four\r", b"echo five\r"], None),
    ("history -d: deleting the edited entry",
     [b"echo one\r", b"echo two\r"] + EDIT + [b"history -d 1\r"],
     # The drop must really happen, or the flow proves nothing: entry 1
     # is the edited one, and `history` must no longer list it.
     ("history\r", "1  echo one")),
    ("history -c: clearing a list holding an edited entry",
     [b"echo one\r", b"echo two\r"] + EDIT + [b"history -c\r"], None),
    ("control: browsing without editing leaks nothing",
     [b"echo one\r", b"echo two\r", b"\x1b[A", b"\x1b[A", b"\x1b[B",
      b"\x1b[B", b"\x15", b"echo one\r"], None),
    ("control: editing without dropping leaks nothing",
     [b"echo one\r", b"echo two\r"] + EDIT + [b"echo three\r"], None),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def is_asan(path):
    """An ASan build carries the runtime's own symbols in the binary."""
    try:
        with open(path, "rb") as f:
            return b"__asan_init" in f.read()
    except OSError:
        return False


class Session:
    def __init__(self, home, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "PS1": "IN#> ", "HISTFILE": os.path.join(home, "hist"),
            "ASAN_OPTIONS": "detect_leaks=1:exitcode=0",
        }
        env.update(extra)
        self.buf = bytearray()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            fcntl.ioctl(0, termios.TIOCSWINSZ,
                        struct.pack("HHHH", 24, 100, 0, 0))
            os.chdir(home)
            os.execve(SHELL, [SHELL, "--norc"], env)
            os._exit(127)
        self.until(lambda: b"#> " in self.buf, 15.0)

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

    def settle(self, quiet=0.2, cap=2.0):
        end = time.monotonic() + cap
        while time.monotonic() < end and self.pump(quiet):
            pass

    def keys(self, data):
        os.write(self.fd, data)
        self.settle(0.15, 1.5)

    def close(self):
        try:
            os.write(self.fd, b"\x05\x15exit\r")
            self.until(lambda: False, 1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def run_flow(label, home, extra, steps, proof):
    s = Session(home, extra)
    for k in steps:
        s.keys(k)
    if proof:
        cmd, gone = proof
        start = len(s.buf)
        s.keys(cmd.encode())
        s.settle(0.3, 3.0)
        out = bytes(s.buf[start:]).decode("utf-8", "replace")
        check("%s: the entry really was dropped" % label, gone not in out,
              "%r still listed" % gone)
    start = len(s.buf)
    # A forked child: its exit runs LSan over the heap it inherited.
    s.keys(b"( exit 0 )\r")
    s.settle(0.3, 3.0)
    out = bytes(s.buf[start:]).decode("utf-8", "replace")
    n = out.count("LeakSanitizer")
    where = ""
    if n:
        for line in out.splitlines():
            if "rl_" in line or "xmalloc" in line:
                where = line.strip()
                break
    check("%s: %s" % (label, steps and "no leak reported"), n == 0,
          "%d report(s) from a forked child  %s" % (n, where))
    s.close()


def main():
    if not is_asan(SHELL):
        print("skip: %s is not an ASan build; leaks in readline's libc heap "
              "have no oracle at SAFE=0 (build with `make all`)"
              % os.path.basename(SHELL))
        sys.exit(0)
    with tempfile.TemporaryDirectory() as base:
        for reader, rx in (("in-process", {}),
                           ("forked", {"HELLISH_RL_FORK": "1"})):
            for name, steps, proof in FLOWS:
                home = tempfile.mkdtemp(dir=base)
                run_flow("%s: %s" % (reader, name), home, rx, steps, proof)
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

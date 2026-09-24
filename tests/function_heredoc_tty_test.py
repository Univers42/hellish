#!/usr/bin/env python3
"""A heredoc in a one-line function body, typed at a prompt, survives.

    P$ f() { cat <<EOF; }
    heredoc> body
    heredoc> EOF
    P$ f
    Segmentation fault                  (3.1.3: the shell itself died)

A function body's heredoc is re-read at every call, so its body must live
on the AST node. A one-line definition is complete before its body is
read, nothing pre-extracted it, and the body was written into the
DEFINING command's redirect slot -- gone by the first call. The script
half of this is pinned by tests/scripts/52_heredoc_logical_line.sh; this
is the half only a terminal shows: the body arrives through readline's
`heredoc>` prompt, a different reader than a script's buffer.

Waits on output, never on the clock: each step returns the moment the
expected text arrives, so the file runs in about a second.

Usage: python3 function_heredoc_tty_test.py [/path/to/hellish]
"""
import os
import pty
import select
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


class Tty:
    def __init__(self):
        self.home = tempfile.mkdtemp(prefix="fnheredoc-")
        with open(os.path.join(self.home, ".hellishrc"), "w") as f:
            f.write("PS1='P$ '\n")
        env = {"HOME": self.home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
               "TERM": "dumb", "LANG": "C.UTF-8", "HELLISH_NO_BANNER": "1",
               "HELLISH_NO_ANIM": "1", "HELLISH_NO_UPDATE_CHECK": "1",
               "ASAN_OPTIONS": "detect_leaks=0"}
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(self.home)
            os.environ.clear()
            os.environ.update(env)
            os.execv(SHELL, [SHELL, "-i"])
            os._exit(127)
        self.out = b""
        self.pos = 0

    def expect(self, needle, timeout=10.0):
        """Read until `needle` appears past what earlier expects consumed,
        then consume through it."""
        end = time.time() + timeout
        while self.out.find(needle, self.pos) < 0 and time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if not r:
                continue
            try:
                d = os.read(self.fd, 65536)
            except OSError:
                break
            if not d:
                break
            self.out += d
        at = self.out.find(needle, self.pos)
        if at < 0:
            return False
        self.pos = at + len(needle)
        return True

    def send(self, s):
        os.write(self.fd, s.encode())

    def close(self):
        try:
            os.write(self.fd, b"exit\r")
        except OSError:
            pass
        end = time.time() + 5
        while time.time() < end:
            pid, st = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                return st
            self.expect(b"\x00never", 0.1)
        os.kill(self.pid, 9)
        return os.waitpid(self.pid, 0)[1]


def main():
    t = Tty()
    ok = t.expect(b"P$ ")
    check("prompt/appears", ok)
    t.send("f() { cat <<EOF; }\r")
    check("definition/asks-for-body", t.expect(b"heredoc> "))
    t.send("one $((6*7))\r")
    t.expect(b"heredoc> ")
    t.send("EOF\r")
    t.expect(b"P$ ")
    t.send("f; f; echo alive=$?\r")
    got = t.expect(b"alive=0")
    body = t.out.decode("utf-8", "replace")
    check("call/shell-survives", got, repr(body[-300:]))
    check("call/body-each-time", body.count("one 42") >= 2, repr(body[-300:]))
    t.send("g() { cat <<'EOF'; }\r")
    t.expect(b"heredoc> ")
    t.send("lit $((1+1))\r")
    t.expect(b"heredoc> ")
    t.send("EOF\r")
    t.expect(b"P$ ")
    mark = len(t.out)
    t.send("g; echo alive2=$?\r")
    ok = t.expect(b"alive2=0")
    after = t.out[mark:].replace(b"\r", b"")
    check("quoted-delimiter/body-stays-literal",
          ok and after.count(b"lit $((1+1))") >= 1 and b"lit 2" not in after,
          repr(after[-200:]))
    st = t.close()
    check("exit/not-signalled", not os.WIFSIGNALED(st),
          "signal %d" % os.WTERMSIG(st) if os.WIFSIGNALED(st) else "")
    print("%d failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

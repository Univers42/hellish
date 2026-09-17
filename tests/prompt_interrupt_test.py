#!/usr/bin/env python3
"""Regression gate: Ctrl-C at the prompt behaves, whichever reader runs.

Field report, right after the in-process reader became the default: "I did
several Ctrl-C and the signals seem completely broken". They were:

    P#> ^C                      nothing -- no ^C, no new prompt
    P#> echo S=$?               hellish: ho: command not found

The interrupt was swallowed, and so were the next keys. In-process the
SIGINT handler set rl_done, but readline only looks at rl_done after a key
has been read: its read() retries on EINTR, so the ^C sat there until the
user typed, and the keys that woke it were consumed by the dying line.
With the prompt animation on, readline waits in a different loop (the
idle hook's select), so the bug hid in exactly the configuration its
author was looking at. Only the forked reader, where ^C simply kills a
child, never had it.

Every scenario runs through four cells -- in-process and forked reader,
static and animated prompt -- and every assertion waits for a COMPUTED
marker (`echo S=$?_$((2+3))` must print `S=130_5`): the echoed command line
never contains the answer, so a swallowed keystroke fails instead of
matching its own echo.

  empty     ^C at an empty prompt echoes ^C once, a new prompt reads a
            whole command, and $? is 130 (bash)
  discard   a half-typed line is dropped, not executed
  burst     three ^C in a row, then a command runs intact
  command   ^C interrupts a foreground `sleep`
  isearch   ^C inside ^R reverse search
  ps2       ^C at a continuation prompt returns to PS1 with $? = 130
  eof       ^D on an empty line still exits

Usage: python3 prompt_interrupt_test.py /path/to/hellish
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
MARK = b"#> "
STATIC = "P#> "
ANIMATED = "\\A\\nP#> "


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "ASAN_OPTIONS": "detect_leaks=0",
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
        self.at = 0
        self.prompt()

    def pump(self, secs):
        r, _, _ = select.select([self.fd], [], [], secs)
        if not r:
            return False
        try:
            d = os.read(self.fd, 65536)
        except OSError:
            return False
        self.buf.extend(d)
        return bool(d)

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def prompt(self, cap=8.0):
        """Wait for a prompt drawn after self.at; move self.at past it."""
        ok = self.until(lambda: MARK in self.buf[self.at:], cap)
        if ok:
            self.at = self.buf.index(MARK, self.at) + len(MARK)
        return ok

    def send(self, data):
        os.write(self.fd, data)

    def expect(self, token, cap=5.0):
        """Wait for `token` after self.at, then for the prompt after it."""
        start = self.at
        ok = self.until(lambda: token in self.buf[start:], cap)
        if ok:
            self.at = self.buf.index(token, start) + len(token)
            self.prompt()
        return ok

    def tail(self):
        return bytes(self.buf[-160:])

    def close(self):
        try:
            os.kill(self.pid, 9)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass


def interrupt(s):
    """^C, then wait until the new prompt is up."""
    s.send(b"\x03")
    return s.prompt(5.0)


def cell(label, home, extra):
    s = Session(home, extra)

    start = s.at
    new_prompt = interrupt(s)
    echoed = s.buf[start:s.at].count(b"^C")
    s.send(b"echo S=$?_$((2+3))\r")
    check("%s: empty: ^C gives a new prompt" % label, new_prompt,
          "no prompt after ^C: %r" % s.tail())
    check("%s: empty: ^C is echoed once" % label, echoed == 1,
          "echoed %d times" % echoed)
    check("%s: empty: next command intact, $? is 130" % label,
          s.expect(b"S=130_5"), repr(s.tail()))

    s.send(b"echo LEAK_$((6*7))")
    s.until(lambda: b"LEAK_$((6*7))" in s.buf[s.at:], 3.0)
    interrupt(s)
    s.send(b"echo D=$?_$((3+4))\r")
    ok = s.expect(b"D=130_7")
    check("%s: discard: half-typed line is not executed" % label,
          ok and b"LEAK_42" not in s.buf, repr(s.tail()))

    s.send(b"\x03\x03\x03")
    s.until(lambda: s.buf[s.at:].count(b"^C") >= 3, 3.0)
    s.pump(0.3)
    s.at = len(s.buf) - len(MARK) if s.buf.endswith(MARK) else s.at
    s.send(b"echo ALIVE_$((3*3))\r")
    check("%s: burst: a command after ^C^C^C runs intact" % label,
          s.expect(b"ALIVE_9"), repr(s.tail()))

    s.send(b"sleep 30\r")
    s.until(lambda: b"sleep 30\r\n" in s.buf[s.at:], 3.0)
    time.sleep(0.3)
    t0 = time.monotonic()
    interrupt(s)
    s.send(b"echo C=$?_$((4+4))\r")
    ok = s.expect(b"C=130_8")
    check("%s: command: ^C stops sleep promptly, $? 130" % label,
          ok and time.monotonic() - t0 < 5.0, repr(s.tail()))

    s.send(b"\x12ech")
    s.until(lambda: b"reverse-i-search" in s.buf[s.at:], 3.0)
    interrupt(s)
    s.send(b"echo ISR_$((1+1))\r")
    check("%s: isearch: ^C inside ^R, then a command runs" % label,
          s.expect(b"ISR_2"), repr(s.tail()))

    s.send(b"echo \"unterminated\r")
    s.until(lambda: b"> " in s.buf[s.at:], 3.0)
    s.pump(0.2)
    interrupt(s)
    s.send(b"echo P2=$?_$((5+5))\r")
    check("%s: ps2: ^C at a continuation prompt, $? 130" % label,
          s.expect(b"P2=130_10"), repr(s.tail()))

    s.send(b"\x04")
    gone = False
    end = time.monotonic() + 5.0
    while time.monotonic() < end and not gone:
        s.pump(0.05)
        gone = os.waitpid(s.pid, os.WNOHANG)[0] != 0
    check("%s: eof: ^D on an empty line exits" % label, gone,
          repr(s.tail()))
    if not gone:
        s.close()


def main():
    with tempfile.TemporaryDirectory() as home:
        for reader, rx in (("in-process", {}),
                           ("forked", {"HELLISH_RL_FORK": "1"})):
            cell(reader + "/static", home,
                 dict(rx, PS1=STATIC, HELLISH_NO_ANIM="1"))
            cell(reader + "/animated", home,
                 dict(rx, PS1=ANIMATED, HELLISH_ANIM="pulse"))
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

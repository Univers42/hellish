#!/usr/bin/env python3
"""Regression gate: a resize never reprints the prompt or the line.

Field report: clicking from one terminal tab to another printed another
prompt per click, on the same row --

    ╰─ ❯ ╰─ ❯ ╰─ ❯ ╰─ ❯ ╰─ ❯                                   03:003:00

-- and zooming in and out stacked right-prompt clocks. Tab switches and
zooms raise SIGWINCH, the size often unchanged or changed by a row.

readline's resize path ends in rl_forced_update_display() whenever
rl_redisplay_function is not rl_redisplay -- and the RPROMPT painter makes
it ours. That redraw starts "at column 0" without carrying the cursor
there, so each SIGWINCH appended a copy of the prompt and line after the
last. rl_winch.c takes SIGWINCH from readline: it redraws only when the
WIDTH changed, and clears the line's rows first.

Every cell runs a two-row prompt with a right prompt, typed text on the
input row, in a 100x24 pty; readers in-process and HELLISH_RL_FORK=1, the
prompt static and animated (the animation's idle hook is readline's other
wait loop, and the redraw must happen from there too).

  same-size  5 SIGWINCH, size unchanged (a tab switch): the input row is
             untouched -- and for a static prompt not ONE byte is written
  rows       24 -> 23 -> 24: same, a row count moves nothing on the line
  width      100 -> 80 -> 100: exactly one copy of the line on screen, the
             cursor right after the text
  burst      ten alternating widths back to back, then a key: the line
             is still one copy and runs as typed
  command    a resize while a command runs: the next prompt is one copy
  trap       with `trap ... WINCH` set the same holds, and the trap still
             runs (the handler chains to it)

Screen assertions use pyte; without it they are reported as skipped and the
wire assertions still run.

Usage: python3 prompt_winch_redraw_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import signal
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []
SKIPS = []
COLS, ROWS = 100, 24
MARK = b"#> "
LINE = "echo w1nch"
RP = "RIGHT 12:34"

try:
    import pyte

    class Screen(pyte.Screen):
        """pyte dispatches CSI with parameters; save/restore take none."""

        def save_cursor(self, *_):
            super().save_cursor()

        def restore_cursor(self, *_):
            super().restore_cursor()

    HAVE_PYTE = True
except ImportError:  # pragma: no cover
    HAVE_PYTE = False


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def screen_check(name, fn):
    if not HAVE_PYTE:
        print("skip " + name + "  (needs pyte)")
        SKIPS.append(name)
        return
    ok, detail = fn()
    check(name, ok, detail)


def winsz(fd, cols, rows):
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))


class Session:
    def __init__(self, home, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HISTFILE": "/dev/null",
            "RPROMPT": RP, "ASAN_OPTIONS": "detect_leaks=0",
        }
        env.update(extra)
        self.buf = bytearray()
        self.cols, self.rows = COLS, ROWS
        self.screen = Screen(COLS, ROWS) if HAVE_PYTE else None
        self.stream = pyte.ByteStream(self.screen) if HAVE_PYTE else None
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            winsz(0, COLS, ROWS)
            os.chdir(home)
            os.execve(SHELL, [SHELL, "--norc"], env)
            os._exit(127)
        self.until(lambda: MARK in self.buf, 10.0)
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
        if self.stream:
            self.stream.feed(d)
        return True

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def settle(self, quiet=0.3, cap=2.0):
        """Read until quiet -- or until `cap`: an animated prompt is never
        quiet, it repaints every tick."""
        end = time.monotonic() + cap
        while time.monotonic() < end and self.pump(quiet):
            pass

    def resize(self, cols, rows):
        """The pty's size, and the terminal model's WIDTH. pyte answers a
        row-count change by deleting screen lines from the top while the
        cursor stays put -- no terminal does that -- so the model keeps
        its rows and a rows-only change is judged on the wire alone."""
        winsz(self.fd, cols, rows)
        self.cols, self.rows = cols, rows
        if self.screen and cols != self.screen.columns:
            self.screen.resize(self.screen.lines, cols)

    def input_row(self):
        return self.screen.display[self.screen.cursor.y]

    def copies(self):
        """Copies of the line on the input row and the row above it --
        the rows a redraw can reach. Earlier prompts further up hold the
        same text legitimately."""
        y = self.screen.cursor.y
        rows = self.screen.display[max(0, y - 1):y + 1]
        return "\n".join(rows).count("#> " + LINE)

    def close(self):
        try:
            os.write(self.fd, b"\x05\x15exit\r")
            self.until(lambda: False, 0.5)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def one_copy(s, label):
    """The line appears once and the cursor sits right after it."""
    def fn():
        row = s.input_row()
        want_x = len("IN#> " + LINE)
        ok = (s.copies() == 1 and row.startswith("IN#> " + LINE)
              and s.screen.cursor.x == want_x)
        return ok, "%s: copies=%d cursor=%s row=%r" % (
            label, s.copies(), (s.screen.cursor.y, s.screen.cursor.x),
            row.rstrip())
    return fn


def cell(label, home, extra, animated):
    s = Session(home, extra)
    os.write(s.fd, LINE.encode())
    s.until(lambda: LINE.encode() in s.buf, 5.0)
    s.settle()
    before = s.input_row() if HAVE_PYTE else None

    start = len(s.buf)
    pgrp = os.tcgetpgrp(s.fd)
    for _ in range(5):
        os.killpg(pgrp, signal.SIGWINCH)
        s.settle(0.1)
    s.settle()
    if not animated:
        check("%s: same-size: SIGWINCH writes nothing" % label,
              len(s.buf) == start, repr(bytes(s.buf[start:])[:120]))
    screen_check("%s: same-size: input row untouched" % label,
                 lambda: (s.input_row() == before and s.copies() == 1,
                          "row=%r copies=%d" % (s.input_row().rstrip(),
                                                s.copies())))

    start = len(s.buf)
    s.resize(COLS, ROWS - 1)
    s.settle(0.15)
    s.resize(COLS, ROWS)
    s.settle()
    if not animated:
        check("%s: rows: a row-count change writes nothing" % label,
              len(s.buf) == start, repr(bytes(s.buf[start:])[:120]))
    screen_check("%s: rows: line and cursor where they were" % label,
                 one_copy(s, "rows"))

    s.resize(80, ROWS)
    s.settle()
    screen_check("%s: width 100->80: one copy of the line" % label,
                 one_copy(s, "80"))
    s.resize(COLS, ROWS)
    s.settle()
    screen_check("%s: width 80->100: one copy of the line" % label,
                 one_copy(s, "100"))

    for i in range(10):
        s.resize(90 if i % 2 else 70, ROWS)
        time.sleep(0.005)
    s.resize(COLS, ROWS)
    s.settle()
    screen_check("%s: burst of resizes: one copy of the line" % label,
                 one_copy(s, "burst"))
    start = len(s.buf)
    os.write(s.fd, b"X\r")
    ok = s.until(lambda: b"w1nchX" in s.buf[start:], 5.0)
    s.until(lambda: MARK in s.buf[s.buf.find(b"w1nchX", start):], 5.0)
    s.settle()
    check("%s: burst: the line still runs as typed" % label, ok,
          repr(bytes(s.buf[start:])[-120:]))

    n = s.buf.count(MARK)
    os.write(s.fd, b"sleep 0.6\r")
    time.sleep(0.2)
    s.resize(80, ROWS)
    s.until(lambda: s.buf.count(MARK) > n, 5.0)
    s.settle()
    os.write(s.fd, LINE.encode())
    s.settle()
    screen_check("%s: command: resize during `sleep`, one copy after" % label,
                 one_copy(s, "command"))
    os.write(s.fd, b"\x05\x15")
    s.settle()

    n = s.buf.count(b"TRAPPED")
    s.resize(COLS, ROWS)
    os.write(s.fd, b"trap 'echo TRAPPED' WINCH\r")
    s.settle()
    os.write(s.fd, LINE.encode())
    s.settle()
    s.resize(80, ROWS)
    s.settle()
    screen_check("%s: trap: one copy with a WINCH trap set" % label,
                 one_copy(s, "trap"))
    os.write(s.fd, b"\x05\x15:\r")
    s.settle()
    check("%s: trap: the WINCH trap still runs" % label,
          s.buf.count(b"TRAPPED") > n, repr(bytes(s.buf[-160:])))
    s.close()


def main():
    with tempfile.TemporaryDirectory() as home:
        for reader, rx in (("in-process", {}),
                           ("forked", {"HELLISH_RL_FORK": "1"})):
            cell(reader + "/static", home,
                 dict(rx, PS1="top\\nIN#> ", HELLISH_NO_ANIM="1"), False)
            cell(reader + "/animated", home,
                 dict(rx, PS1="\\A\\nIN#> ", HELLISH_ANIM="pulse"), True)
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear" + (" (%d skipped)" % len(SKIPS) if SKIPS else ""))


main()

#!/usr/bin/env python3
"""Regression gate: the right prompt never leaves remnants or hides the line.

Field reports, both from history recall with a right prompt set:

    ❯ clear                                   hellihellish perf/readline-inproc
    ❯ echo LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL          (the rest of the command:
                                                     gone from the screen)

readline shortens a line with DCH (ESC[nP), which pulls everything right of
the cursor n columns left, the painted clock included; repainting it at its
column left the pulled copy's head behind. And hiding the clock cleared from
the clock's column to the end of the row, erasing the tail of a recalled
command that already reached past it -- the command that ran was not the one
on screen. rl_rprompt.c now clears from the end of the TEXT (an upper bound
on its width) and repaints.

Rather than a few golden screens, every step asserts one invariant on the
input row, in a terminal model (pyte), after the edit has been displayed:

    row == prompt + text, padded, then EITHER nothing OR the right prompt,
           exactly once, flush with the right edge

and the clock is present exactly when the painter's rule says it fits
(the text's width bound plus one blank stays left of it). The edits are the
ones that move text sideways: history recall up and down across short,
empty, past-the-clock and wide-character entries; backspace in the middle;
^W; ^A^K; ^U; typing into the clock's area and deleting back out of it.
Finally the long recalled command is run, and its output must be the whole
command -- what ran is what was shown.

Cells: readers in-process and HELLISH_RL_FORK=1, right prompt plain and
coloured (%F{..}, whose escapes must not count as width).

Usage: python3 rprompt_edit_test.py /path/to/hellish
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
COLS, ROWS = 100, 30
PROMPT = "IN#> "
RP_TEXT = "hellish perf/readline-inproc ~10 ?2 02:57"
LONG = "echo " + "L" * 70
WIDE = "echo été 日本"

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


def byte_bound(text):
    """The painter's upper bound on the columns `text` takes."""
    n = 0
    for b in text.encode():
        if b == 9:
            n += 8
        elif b < 32 or b == 127:
            n += 2
        else:
            n += 1
    return n


class Session:
    def __init__(self, home, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "HISTFILE": "/dev/null", "PS1": PROMPT,
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        env.update(extra)
        self.buf = bytearray()
        self.screen = Screen(COLS, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            fcntl.ioctl(0, termios.TIOCSWINSZ,
                        struct.pack("HHHH", ROWS, COLS, 0, 0))
            os.chdir(home)
            os.execve(SHELL, [SHELL, "--norc"], env)
            os._exit(127)
        self.until(lambda: b"#> " in self.buf, 10.0)
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
        self.stream.feed(d)
        return True

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def settle(self, quiet=0.25, cap=3.0):
        end = time.monotonic() + cap
        while time.monotonic() < end and self.pump(quiet):
            pass

    def run(self, line):
        n = self.buf.count(b"#> ")
        os.write(self.fd, line.encode() + b"\r")
        self.until(lambda: self.buf.count(b"#> ") > n, 8.0)
        self.settle()

    def keys(self, data):
        os.write(self.fd, data)
        self.settle()

    def cells(self):
        """The input row as one string per cell; a wide character's second
        cell is '' in pyte, so indices are columns."""
        y = self.screen.cursor.y
        line = self.screen.buffer[y]
        return [line[x].data for x in range(COLS)]

    def close(self):
        try:
            os.write(self.fd, b"\x05\x15exit\r")
            self.until(lambda: False, 0.5)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def invariant(s, label, text, rp_w):
    """The input row holds exactly the prompt and `text`, then either
    blanks or the right prompt once at the right edge, the latter exactly
    when it fits by the painter's rule."""
    cells = s.cells()
    end = len(PROMPT) + byte_bound(text)
    fits = COLS > rp_w + 1 and end + 1 < COLS - rp_w
    left = "".join(cells[:COLS - rp_w]).rstrip()
    right = "".join(cells[COLS - rp_w:])
    whole = "".join(cells).rstrip()
    want = (PROMPT + text).rstrip()
    if fits:
        ok = left == want and right == RP_TEXT
    else:
        ok = whole == want
    check("%s: %s" % (label, "row is prompt+text, clock %s"
                      % ("once at the edge" if fits else "hidden")),
          ok, "row=%r" % whole)


def cell(label, home, extra):
    rp_w = len(RP_TEXT)
    s = Session(home, extra)
    for line in (WIDE, LONG, "echo one", "echo tw"):
        s.run(line)
    s.run("clear")
    time.sleep(0.2)
    s.settle()

    history = ["clear", "echo tw", "echo one", LONG, WIDE]
    shown = []
    for i in range(len(history)):
        s.keys(b"\x1b[A")
        shown.append(history[i])
        invariant(s, "%s: up %d (%r)" % (label, i + 1, history[i][:12]),
                  history[i], rp_w)
    for i in range(len(history) - 2, -1, -1):
        s.keys(b"\x1b[B")
        invariant(s, "%s: down to %r" % (label, history[i][:12]),
                  history[i], rp_w)
    s.keys(b"\x1b[B")
    invariant(s, "%s: down to the empty line" % label, "", rp_w)

    s.keys(b"echo abcdef")
    s.keys(b"\x1b[D\x1b[D\x7f\x7f")
    invariant(s, "%s: backspace mid-line" % label, "echo abef", rp_w)
    s.keys(b"\x05 aa bb cc")
    s.keys(b"\x17\x17")
    invariant(s, "%s: ^W twice" % label, "echo abef aa ", rp_w)
    s.keys(b"\x01\x0b")
    invariant(s, "%s: ^A^K" % label, "", rp_w)

    s.keys(b"x" * 60)
    invariant(s, "%s: typed into the clock's area" % label, "x" * 60, rp_w)
    s.keys(b"\x7f" * 50)
    invariant(s, "%s: deleted back out of it" % label, "x" * 10, rp_w)
    s.keys(b"\x15")
    invariant(s, "%s: ^U" % label, "", rp_w)

    for _ in range(3):
        s.keys(b"\x1b[A")
    s.keys(b"\x1b[A")
    invariant(s, "%s: long command recalled" % label, LONG, rp_w)
    start = len(s.buf)
    s.run("")
    out = bytes(s.buf[start:]).decode("utf-8", "replace")
    check("%s: the recalled command runs whole" % label,
          ("L" * 70) in out.replace("\r", "").split("\n", 1)[-1],
          repr(out[-160:]))
    s.close()


def main():
    if not HAVE_PYTE:
        print("skip: needs pyte (python3 -m pip install pyte)")
        sys.exit(0)
    with tempfile.TemporaryDirectory() as home:
        for reader, rx in (("in-process", {}),
                           ("forked", {"HELLISH_RL_FORK": "1"})):
            cell(reader + "/plain", home, dict(rx, RPROMPT=RP_TEXT))
            cell(reader + "/coloured", home,
                 dict(rx, RPROMPT="%F{244}" + RP_TEXT + "%f"))
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

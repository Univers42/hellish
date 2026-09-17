#!/usr/bin/env python3
"""Regression gate: every prompt is drawn at the terminal's CURRENT width.

Field report, every theme at once: after resizing the terminal the typed
line overwrote its own prompt (`fffff/hellish perf/... ▸ ffff`), the
right-prompt clock and the box rule wrapped onto an extra row, and
`theme next` drifted a little further on each prompt.

One commit hoisted rl_initialize() out of the per-prompt readline child
into the shell, and rl_initialize() measures the terminal. From then on
there were three stale copies of "how wide is the terminal":

  readline   _rl_screenwidth, measured once at startup. It decides where
             the line wraps (readline writes its wrap as a bare \\r, which on
             a row that has not really wrapped rewrites the prompt) and the
             column the RPROMPT painter jumps to. A forked reader caught the
             SIGWINCH and then exited, taking the new size with it.
  environ    readline setenv()s COLUMNS/LINES while measuring, and get_cols()
             -- the \\B box, the mascot -- honours an inherited COLUMNS.
             In the shell's own process that pinned it to the old width.
  $COLUMNS   refreshed only before an execution, so a bare Enter after a
             resize drew PS1 with the old value. bash updates it on the
             resize itself.

Each is asserted here on the wire, no terminal model needed, for both read
paths (in-process and HELLISH_RL_FORK=1) and both moments a resize can
happen: while the prompt waits (the harness sets the pty size) and while a
command runs (`stty cols N` inside the shell -- the kernel raises SIGWINCH
with no readline in sight).

  WRAP     after the resize, type a line that wraps at exactly one of the
           widths the session has had, then Ctrl-A. readline climbs a row
           (ESC[A) iff it believes the line wrapped; that belief is what
           places the cursor.
  RPROMPT  the painter's ESC[<n>G column is cols - width + 1.
  VARS     PS1 shows $COLUMNS:$LINES at the new size after a bare Enter; a
           hand-set COLUMNS survives a bare Enter until the next resize.
  BOX      the \\B theme's top row follows the width, keeping the margin
           it had at startup.

The "during a command" checks read the prompt drawn right after `stty`
returns -- the one the user sees -- never a later one, which a stale
render would have had a prompt's worth of time to heal.

Usage: python3 prompt_resize_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
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
BOX_MARK = "❯".encode()
ANSI = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]|\x1b[78]|\x1b\][^\x07]*\x07"
                  r"|[\x01\x02\r]")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def winsz(fd, cols, rows):
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))


class Session:
    def __init__(self, env, cols, rows, mark):
        self.buf = bytearray()
        self.mark = mark
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            winsz(0, cols, rows)
            os.chdir(env["HOME"])
            os.execve(SHELL, [SHELL, "--norc"], env)
            os._exit(127)
        self.first = self.prompt()

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

    def until(self, pred, cap=8.0):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def prompt(self, send=b""):
        """Send, then wait for one MORE prompt marker; return what came."""
        start = len(self.buf)
        n = self.buf.count(self.mark)
        if send:
            os.write(self.fd, send)
        self.until(lambda: self.buf.count(self.mark) > n)
        self.settle()
        return bytes(self.buf[start:])

    def settle(self, quiet=0.25):
        """Let the redisplay that follows a prompt (RPROMPT) arrive. A
        correctness read, not a timing one: nothing here is measured."""
        while self.pump(quiet):
            pass

    def ctrl_a_after(self, n):
        """Type n chars, wait for their echo, then Ctrl-A; its output."""
        start = len(self.buf)
        os.write(self.fd, b"x" * n)
        self.until(lambda: self.buf[start:].count(b"x") >= n)
        self.settle()
        start = len(self.buf)
        os.write(self.fd, b"\x01")
        self.settle()
        out = bytes(self.buf[start:])
        os.write(self.fd, b"\x05\x15")
        self.settle()
        return out

    def close(self):
        try:
            os.write(self.fd, b"\x15exit\r")
            self.until(lambda: False, 1.0)
        except OSError:
            pass
        try:
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def base_env(home, extra):
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
        "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
        "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    env.update(extra)
    return env


def rprompt_col(out):
    """Column of the last clock painted. The frame may clear from the end
    of the text first (ESC[<n>G ESC[K), so match the jump the clock itself
    follows, not the frame's first one."""
    cols = re.findall(rb"\x1b\[(\d+)GRP\x1b8", out)
    return int(cols[-1]) if cols else None


def readline_cell(path, home, extra):
    """WRAP, RPROMPT and VARS through one session: 80x24, widened to
    140x30 at the prompt, then shrunk to 60 by a running command."""
    env = base_env(home, dict(extra, PS1="W$COLUMNS:$LINES#> ", RPROMPT="RP"))
    s = Session(env, 80, 24, MARK)

    winsz(s.fd, 140, 30)
    s.settle()
    out = s.prompt(b"\r")
    check("%s: at the prompt, widen: PS1 sees 140:30" % path,
          b"W140:30#> " in out, repr(out[-80:]))
    col = rprompt_col(out)
    check("%s: at the prompt, widen: RPROMPT painted at column 139" % path,
          col == 139, "column %r" % col)
    a = s.ctrl_a_after(100)
    check("%s: at the prompt, widen: 110 columns do not wrap at 80" % path,
          b"\x1b[A" not in a, "readline still wraps at 80: %r" % a[:80])

    out = s.prompt(b"stty cols 60\r")
    check("%s: during a command, shrink: PS1 sees 60:30" % path,
          b"W60:30#> " in out, repr(out[-80:]))
    col = rprompt_col(out)
    check("%s: during a command, shrink: RPROMPT at column 59" % path,
          col == 59, "column %r" % col)
    a = s.ctrl_a_after(60)
    check("%s: during a command, shrink: 69 columns wrap at 60" % path,
          b"\x1b[A" in a, "readline believes an older width: %r" % a[:80])

    s.prompt(b"COLUMNS=33\r")
    out = s.prompt(b"\r")
    check("%s: a hand-set COLUMNS survives a bare Enter" % path,
          b"W33:30#> " in out, repr(out[-80:]))
    winsz(s.fd, 70, 20)
    s.settle()
    out = s.prompt(b"\r")
    check("%s: ... until the terminal really resizes" % path,
          b"W70:20#> " in out, repr(out[-80:]))
    s.close()


def box_top_width(out):
    """Width of the row printed just before the \\B box's input row."""
    rows = out.split(b"\n")
    for i in range(len(rows) - 1, -1, -1):
        if BOX_MARK in rows[i] and i > 0:
            return len(ANSI.sub("", rows[i - 1].decode("utf-8", "replace")))
    return None


def box_cell(path, home, extra):
    """BOX: get_cols() must measure the terminal, not a startup COLUMNS
    that readline exported into the shell's own environment. The box
    keeps a margin by design; it is measured at startup, not assumed."""
    env = base_env(home, dict(extra, PS1="\\B"))
    s = Session(env, 140, 24, BOX_MARK)
    w = box_top_width(s.first)
    margin = 140 - w if w else 0

    winsz(s.fd, 80, 24)
    s.settle()
    w = box_top_width(s.prompt(b"\r"))
    check("%s: \\B box follows a resize at the prompt (80)" % path,
          w == 80 - margin,
          "top row is %r columns, want %d" % (w, 80 - margin))
    w = box_top_width(s.prompt(b"stty cols 100\r"))
    check("%s: \\B box follows a resize during a command (100)" % path,
          w == 100 - margin,
          "top row is %r columns, want %d" % (w, 100 - margin))
    s.close()


def main():
    with tempfile.TemporaryDirectory() as home:
        for path, extra in (("in-process", {}),
                            ("forked", {"HELLISH_RL_FORK": "1"})):
            readline_cell(path, home, extra)
            box_cell(path, home, extra)
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

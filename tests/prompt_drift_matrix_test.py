#!/usr/bin/env python3
"""Regression gate: the prompt never drifts the cursor, whatever it is made of.

Three oracles, none of which trusts hellish's own width model, run over a
matrix of prompt shapes in a real pty:

  (a) SCREEN   -- press Up (recall the previous command) then Down (back to
                  the empty line). The screen and cursor must be EXACTLY what
                  they were before Up. A terminal emulator (pyte) is the judge.
  (b) BELIEF   -- type 90 characters into a 100-column terminal, then Ctrl-A.
                  readline walks back to the prompt with backspaces or a
                  carriage return; if it emits ESC[A it believes the line
                  wrapped, i.e. it counted phantom prompt columns. That is the
                  number that places the cursor, so it is the one that matters.
  (c) WIRE     -- \\001 and \\002 are readline's private width markers. Not one
                  of them may reach the terminal.

Field report (cursor drift on history recall with a configured ~/.hellishrc):
the culprit was RPROMPT. prompt_rprompt.c appended the rendered right prompt
INSIDE readline's prompt string, guarded by one \\001...\\002 pair -- but a
coloured RPROMPT already carries its own pairs, and readline does not nest:
it stopped ignoring at the first \\002, counted the rest as visible, and
copied the inner markers to the tty. Every RPROMPT cell below was red; every
cell without one was green, including the coloured two-row prompt with
multi-line recall. The plain (uncoloured) RPROMPT drifted nothing but was
ERASED by readline's clear-to-end-of-line on every redraw.

pyte lacks SCOSC/SCORC (ESC[s / ESC[u), which is exactly the pair RPROMPT
uses; without teaching it those, oracle (a) blames the terminal model for the
shell's bug. When pyte is missing, (b) and (c) still run -- they need no
terminal model at all -- and (a) is reported as skipped, not passed.

Usage: python3 prompt_drift_matrix_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURE = os.path.join(HERE, "fixtures", "frontend.hellishrc")
BASH = shutil.which("bash")
COLS, ROWS = 100, 24
FAILS = []

try:
    import pyte

    class Screen(pyte.Screen):
        """pyte dispatches CSI with parameters; save/restore take none."""

        def save_cursor(self, *_):
            super().save_cursor()

        def restore_cursor(self, *_):
            super().restore_cursor()

    pyte.Stream.csi = dict(pyte.Stream.csi, s="save_cursor", u="restore_cursor")
    HAVE_PYTE = True
except ImportError:  # pragma: no cover
    HAVE_PYTE = False


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


ONE_ROW = "\\$ "
TWO_ROW = "top\\n\\$ "
COLOURED = ("\\[\\e[38;5;238m\\]╭─\\[\\e[0m\\] \\w\\n"
            "\\[\\e[38;5;238m\\]╰─\\[\\e[0m\\] \\[\\e[38;5;114m\\]❯\\[\\e[0m\\] ")
PLAIN_RP = "14:42"
COLOUR_RP = "%F{238}14:42%f"
MULTILINE = [b"for i in 1 2\r", b"do echo $i\r", b"done\r", b"\r"]


class Session:
    def __init__(self, argv, env, cwd):
        self.buf = bytearray()
        self.screen = Screen(COLS, ROWS) if HAVE_PYTE else None
        self.stream = pyte.ByteStream(self.screen) if HAVE_PYTE else None
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd)
            os.environ.clear()
            os.environ.update(env)
            os.execvp(argv[0], argv)
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", ROWS, COLS, 0, 0))

    def drain(self, cap=3.0, quiet=0.3):
        """Read until the pty is quiet for `quiet` s; return the new bytes."""
        start = len(self.buf)
        last = time.monotonic()
        end = last + cap
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    break
                if not d:
                    break
                self.buf.extend(d)
                if self.stream:
                    self.stream.feed(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                break
        return bytes(self.buf[start:])

    def send(self, data, cap=3.0):
        os.write(self.fd, data)
        return self.drain(cap)

    def snapshot(self):
        s = self.screen
        return (s.cursor.y, s.cursor.x, tuple(l.rstrip() for l in s.display))

    def close(self):
        try:
            os.write(self.fd, b"exit\r")
            self.drain(1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def base_env(home):
    return {
        "HOME": home, "XDG_CONFIG_HOME": os.path.join(home, ".config"),
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }


def probe(label, s, pre):
    """Run the three oracles on an already-started session."""
    for c in pre:
        s.send(c)
    for c in MULTILINE:
        s.send(c)
    if s.screen:
        before = s.snapshot()
    s.send(b"\x1b[A")
    s.send(b"\x1b[B")
    if s.screen:
        after = s.snapshot()
        same = before[:2] == after[:2] and before[2] == after[2]
        check("%s: Up then Down restores the screen" % label, same,
              "before cursor=%s row=%r / after cursor=%s row=%r"
              % (before[:2], before[2][before[0]], after[:2],
                 after[2][after[0]]))
    else:
        print("skip %s: screen oracle needs pyte" % label)
    s.send(b"x" * 90)
    out = s.send(b"\x01")
    check("%s: Ctrl-A after 90 chars does not climb a row" % label,
          b"\x1b[A" not in out,
          "readline believes the prompt is wider than it is: %r" % out[:120])
    s.send(b"\x15")
    leaked = s.buf.count(b"\x01") + s.buf.count(b"\x02")
    check("%s: no raw \\x01/\\x02 marker reaches the terminal" % label,
          leaked == 0, "%d marker bytes on the wire" % leaked)


def hellish_cell(label, cwd, home, ps1, rprompt, multiline, rc=None):
    env = base_env(home)
    argv = [SHELL]
    if rc:
        shutil.copy(rc, os.path.join(home, ".hellishrc"))
    else:
        env["PS1"] = ps1
        if rprompt:
            env["RPROMPT"] = rprompt
        argv.append("--norc")
    s = Session(argv, env, cwd)
    s.drain(6.0)
    pre = [b"pretty on multiline-history\r"] if multiline and not rc else []
    probe(label, s, pre)
    s.close()
    if rc:
        os.unlink(os.path.join(home, ".hellishrc"))


def bash_control(cwd, home):
    if not BASH:
        print("skip bash control: bash not found")
        return
    env = dict(base_env(home), PS1=ONE_ROW)
    s = Session([BASH, "--norc", "--noprofile"], env, cwd)
    s.drain(4.0)
    probe("bash control", s, [b"shopt -s cmdhist lithist\r"])
    s.close()


def theme_switch(cwd, home):
    """PS1 goes from two rows to one mid-session (what `theme next` does);
    the next prompt must sit on its own row, right under the command's
    output, with the cursor just after it."""
    if not HAVE_PYTE:
        print("skip theme switch: needs pyte")
        return
    env = dict(base_env(home), PS1=COLOURED, RPROMPT=COLOUR_RP)
    s = Session([SHELL, "--norc"], env, cwd)
    s.drain(6.0)
    s.send(b"PS1='\\$ '\r")
    s.send(b"echo SWITCHED\r")
    y, x, rows = s.snapshot()
    # The right prompt may legitimately sit at the end of the new row; what
    # must not happen is the prompt landing on the output's row or a row
    # of its own being skipped.
    check("theme switch: prompt lands on the row after the output",
          y > 0 and rows[y - 1].endswith("SWITCHED") and rows[y].startswith("$"),
          "cursor=(%d,%d) row=%r above=%r" % (y, x, rows[y], rows[y - 1]))
    check("theme switch: cursor sits right after the new prompt",
          x == 2, "col=%d" % x)
    s.close()


def interrupt_case(cwd, home):
    """^C on a typed line: the fresh prompt follows on the next rows, with
    no stray blank row between (field report: a lone space on its own line)."""
    if not HAVE_PYTE:
        print("skip interrupt: needs pyte")
        return
    shutil.copy(FIXTURE, os.path.join(home, ".hellishrc"))
    s = Session([SHELL], base_env(home), cwd)
    s.drain(6.0)
    s.send(b"git clone")
    y0 = s.screen.cursor.y
    s.send(b"\x03")
    y1 = s.screen.cursor.y
    check("interrupt: new two-row prompt is exactly 2 rows below the ^C line",
          y1 - y0 == 2, "rows moved=%d" % (y1 - y0))
    s.close()
    os.unlink(os.path.join(home, ".hellishrc"))


def main():
    base = tempfile.mkdtemp(prefix="hellish_drift_")
    home = os.path.join(base, "home")
    cwd = os.path.join(base, "work")
    os.makedirs(home)
    os.makedirs(cwd)
    print("pyte terminal model:", "yes" if HAVE_PYTE else "NO (screen oracle skipped)")

    bash_control(cwd, home)
    for ml in (False, True):
        tag = "lithist" if ml else "cmdhist"
        hellish_cell("one-row / no RPROMPT / %s" % tag, cwd, home, ONE_ROW, None, ml)
        hellish_cell("one-row / plain RPROMPT / %s" % tag, cwd, home, ONE_ROW, PLAIN_RP, ml)
        hellish_cell("one-row / coloured RPROMPT / %s" % tag, cwd, home, ONE_ROW, COLOUR_RP, ml)
        hellish_cell("two-row / no RPROMPT / %s" % tag, cwd, home, TWO_ROW, None, ml)
        hellish_cell("two-row / coloured RPROMPT / %s" % tag, cwd, home, TWO_ROW, COLOUR_RP, ml)
        hellish_cell("coloured two-row / no RPROMPT / %s" % tag, cwd, home, COLOURED, None, ml)
        hellish_cell("coloured two-row / coloured RPROMPT / %s" % tag, cwd, home, COLOURED, COLOUR_RP, ml)
    hellish_cell("fixture rc (frontend.hellishrc)", cwd, home, None, None, True, rc=FIXTURE)
    theme_switch(cwd, home)
    interrupt_case(cwd, home)

    shutil.rmtree(base, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

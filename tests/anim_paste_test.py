#!/usr/bin/env python3
"""Regression test: the prompt animation must never clobber pasted input.

Guards the anim_line_fits gate in mascot_anim.c. The old repaint climb
was count_nl + (lastw + rl_point) / cols with rl_point in BYTES: a paste
carrying multibyte glyphs overshot (climbing into scrollback and erasing
it) and a paste with a trailing newline undershot (ESC[2K landing on the
input row, eating the paste and the arrow). The fix freezes the idle
repaint whenever the cursor may have left the input's first screen row.

Checks, in a real pty (100 cols) with HELLISH_ANIM=pulse and a 2-row PS1:
  1. Idle at the prompt, repaint frames flow (animation is alive).
  2. Every cursor-up in every frame climbs exactly 2 rows (count_nl).
  3. Bracketed paste with a trailing newline freezes the repaint, and
     the line still executes intact on Enter.
  4. A paste wide enough to wrap freezes the repaint, executes intact.
  5. A short ASCII line resumes the animation (gate is not just "off").

Usage: python3 anim_paste_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import struct
import sys
import termios
import fcntl
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
PS1 = ("\\n\\[\x1b[36m\\]\\w\\[\x1b[0m\\] \\A\\n"
       "\\[\x1b[35m\\]> \\[\x1b[0m\\]")
FRAME = b"\x1b[?2026h"
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_ANIM": "pulse", "PS1": PS1,
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.raw = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            # --norc: pin the config. An inherited ~/.hellishrc can set PS1 or
            # define names, and quietly decide what this test sees.
            os.execvp(SHELL, [SHELL, "--norc"])
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 24, 100, 0, 0))
        self.drain(0.8)

    def drain(self, t=0.35):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.08)
            if r:
                try:
                    self.raw += os.read(self.fd, 65536)
                except OSError:
                    return

    def frames_during(self, data, t):
        n0 = self.raw.count(FRAME)
        os.write(self.fd, data)
        self.drain(t)
        return self.raw.count(FRAME) - n0

    def close(self):
        try:
            os.write(self.fd, b"\x04")
            self.drain(0.3)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def main():
    import tempfile
    home = tempfile.mkdtemp(prefix="hellish_anim_")
    s = Session(home)

    idle = s.frames_during(b"", 0.7)
    check("idle prompt animates", idle >= 2, "frames=%d" % idle)

    # trailing newline inside the bracketed paste (whole-line copy):
    # repaint must freeze (<=1 tolerates one tick racing the insert)
    n = s.frames_during(b"\x1b[200~echo NL_MARKER\n\x1b[201~", 0.8)
    check("trailing-\\n paste freezes repaint", n <= 1, "frames=%d" % n)
    s.frames_during(b"\r", 0.5)
    check("trailing-\\n paste executes intact",
          re.search(rb"\rNL_MARKER\r\n", s.raw) is not None)

    # wide paste that wraps the input row: freeze, then execute intact
    wide = b"echo W_" + b"x" * 120
    n = s.frames_during(b"\x1b[200~" + wide + b"\x1b[201~", 0.8)
    check("wrapped paste freezes repaint", n <= 1, "frames=%d" % n)
    s.frames_during(b"\r", 0.5)
    check("wrapped paste executes intact",
          re.search(rb"\rW_x{120}\r\n", s.raw) is not None)

    # short ASCII line: the gate must let the animation keep running
    n = s.frames_during(b"echo ok", 0.7)
    check("short line keeps animating", n >= 2, "frames=%d" % n)
    s.frames_during(b"\r", 0.4)

    # multibyte paste: whether it animates depends on the locale, but it
    # must execute intact and never change the climb height (next check)
    s.frames_during("\x1b[200~echo ✦MB_MARKER\x1b[201~".encode(), 0.6)
    s.frames_during(b"\r", 0.5)
    check("multibyte paste executes intact",
          re.search(b"\\r" + "✦MB_MARKER".encode() + b"\\r\\n",
                    s.raw) is not None)

    # continuation prompt (PS2): the armed frames describe the PS1 block,
    # but the rows above a dquote> cursor are the user's own input lines --
    # the repaint must stay disarmed for the whole multiline read
    # (<=1 tolerates one armed tick racing the accept of the first line)
    s.frames_during(b'echo "', 0.3)
    n = s.frames_during(b"\r", 0.7)
    check("continuation read never repaints", n <= 1, "frames=%d" % n)
    s.frames_during(b'L2"\r', 0.5)
    check("multiline executes intact",
          re.search(rb"\r\nL2\r\n", s.raw) is not None)

    # heredoc body lines are readline forks too -- same single-shot rule
    s.frames_during(b"cat <<EOF\r", 0.2)
    n = s.frames_during(b"", 0.6)
    check("heredoc read never repaints", n <= 1, "frames=%d" % n)
    s.frames_during(b"HD_BODY\rEOF\r", 0.6)
    check("heredoc executes intact",
          re.search(rb"\rHD_BODY\r\n", s.raw) is not None)

    ups = set(int(m) for m in re.findall(rb"\x1b\[(\d+)A", s.raw))
    check("every repaint climbs exactly 2 rows", ups <= {2}, str(ups))

    s.close()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

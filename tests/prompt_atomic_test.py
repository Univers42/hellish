#!/usr/bin/env python3
"""Regression test: the prompt prefix must reach the tty in ONE write().

Guards split_prompt() in src/platform/posix/rl.c.

Between one readline returning (which restores the terminal, so kernel echo
is back on) and the next readline entering raw mode, the shell writes every
prompt line but the last itself.  That write used to stream through
unbuffered fputc on stderr -- one write() syscall PER BYTE.  The tty line
discipline echoes type-ahead between two user-space writes, so a key pressed
in that window landed INSIDE a colour escape.  Every ASCII letter is a valid
CSI final byte (0x40-0x7e), so the escape terminated early and the terminal
printed the remainder as literal text:

    > 38;2;112
    > ;79;87m

which is what issues #10 and #19 reported.  The same split lands inside the
prompt's multibyte box-drawing characters, and the terminal shows the
mangled remains as replacement glyphs -- issue #5.  A third symptom comes
free: the echoed byte is *eaten* by the escape parser, so typing `ls` shows
only the `s` while the command still runs, exactly as #10 describes.

Composing the prefix in memory and delivering it in one write closes the
window -- the same reason mascot_redraw.c builds its frame before writing it.

The race is timing-dependent, so we force it: type single bytes at random
sub-millisecond intervals for several seconds while the shell keeps cycling
prompts, with a PS1 whose first row is a few hundred bytes of truecolor
escapes (the shipped default prompt is that shape).  Before the fix this
scores hundreds of fragments in a few seconds; after it, zero.

Usage: python3 prompt_atomic_test.py /path/to/hellish [seconds]
"""
import fcntl
import os
import pty
import random
import re
import select
import struct
import sys
import shutil
import tempfile
import termios
import threading
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
SECS = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0
BLUE = "\\[\x1b[38;2;122;162;247m\\]"
# escapes AND box-drawing glyphs, like the shipped default prompt: the first
# exposes a split CSI, the second a split UTF-8 sequence
PS1 = ("\\n" + (BLUE + "\u2500\u2500\u2500-=-") * 24 + "\\[\x1b[0m\\]\\n"
       + "\\[\x1b[38;2;152;195;121m\\]> \\[\x1b[0m\\]")
# a complete, well-formed CSI sequence -- what the terminal should receive
CSI = re.compile(rb"\x1b\[[0-9;?]*[ -/]*[@-~]")
# a bare SGR tail, i.e. an escape the terminal would have shown as text
TAIL = re.compile(rb"[0-9;]{2,}m|38;2;[0-9;]*")
FAILS = []


def broken_utf8(buf):
    """Count U+2500 box-drawing characters that no longer decode.

    A whole-line read can legitimately cut the tail off the buffer, so we
    only look at the bytes that follow a valid 0xe2 0x94 lead-in.
    """
    n = 0
    i = 0
    while True:
        i = buf.find(b"\xe2\x94", i)
        if i < 0 or i + 2 >= len(buf):
            return n
        if not 0x80 <= buf[i + 2] <= 0xbf:
            n += 1
        i += 2


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else " " + detail))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": PS1,
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        }
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.raw = bytearray()
        self.stop = False
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            # --norc: pin the config. An inherited ~/.hellishrc can set PS1 or
            # define names, and quietly decide what this test sees.
            os.execvp(SHELL, [SHELL, "--norc"])
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 40, 200, 0, 0))
        self.rd = threading.Thread(target=self.reader, daemon=True)
        self.rd.start()
        time.sleep(0.9)

    def reader(self):
        while not self.stop:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    self.raw.extend(os.read(self.fd, 65536))
                except OSError:
                    return

    def hammer(self, secs):
        """Type at random phases so some bytes land inside a prompt write."""
        end = time.time() + secs
        n = 0
        while time.time() < end:
            if n % 7 == 0:
                out = b"echo MARK\n"
            else:
                out = random.choice(b"abcdefgh").to_bytes(1, "big")
            try:
                os.write(self.fd, out)
            except OSError:
                break
            n += 1
            time.sleep(random.uniform(0.0001, 0.0015))
        return n

    def close(self):
        try:
            os.write(self.fd, b"\x03\nexit\n")
        except OSError:
            pass
        time.sleep(0.7)
        self.stop = True
        self.rd.join(timeout=1)
        try:
            os.close(self.fd)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except OSError:
            pass


def main():
    home = tempfile.mkdtemp(prefix="hellish_prompt_")
    s = Session(home)
    writes = s.hammer(SECS)
    s.close()
    shutil.rmtree(home, ignore_errors=True)
    data = bytes(s.raw)
    frags = TAIL.findall(CSI.sub(b"", data))
    check("shell stayed alive under the type-ahead storm",
          len(data) > 10000 and writes > 500,
          "bytes=%d writes=%d" % (len(data), writes))
    check("the typed commands actually ran",
          data.count(b"MARK") > 20, "MARK seen %d times" % data.count(b"MARK"))
    check("no colour escape was split by echoed type-ahead",
          not frags,
          "%d fragments, first: %r" % (len(frags), frags[:5]))
    bad = broken_utf8(CSI.sub(b"", data))
    check("no box-drawing glyph was split by echoed type-ahead",
          not bad, "%d mangled multibyte sequences" % bad)
    print("\n%d/%d checks passed (%d bytes, %d writes)"
          % (4 - len(FAILS), 4, len(data), writes))
    sys.exit(1 if FAILS else 0)


main()

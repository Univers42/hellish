#!/usr/bin/env python3
"""Regression gate: no escape sequence reaches the tty split across write()s.

prompt_atomic_test.py hammers a live prompt with type-ahead and counts the
colour escapes it finds broken. That is the symptom, and it is a race: on
an idle machine it passes, on a loaded CI runner it fails now and then --
it failed on develop at a2b7f1b, in the prompt's LAST row, the one row
rl_prerow.c's fix could not account for.

This file checks the cause instead, and the cause is not timing. Linux's
line discipline keeps the echo of a key it could not put out yet (the
reader was behind) and emits it at the start of the NEXT write() on the
tty -- whatever ECHO is by then (n_tty_write -> process_echoes). So every
write() boundary is a place a typed key can appear, and a boundary inside
`\\e[38;2;152;195;121m` is a place it splits the escape: every letter is a
CSI final byte, and the tail prints as text (issues #10, #19; #5 for a
split UTF-8 glyph).

readline drew the prompt through an unbuffered stderr, one putc per byte
-- 26 write()s for `\\e[38;2;152;195;121m> \\e[0m` -- so its row was all
boundaries. bash line-buffers stderr (shell_initialize) and its row goes
out in one write(). The invariant pinned here:

  no write() boundary falls inside an escape sequence or a UTF-8 character
  anywhere in what the shell sends the terminal, over several prompts, a
  typed line, and a redraw.

It holds on any machine or none: the answer is in strace's record of the
write() calls, not in a race. Skips (exit 0, says so) without strace; CI
installs it.

Usage: python3 prompt_row_writes_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import shutil
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
STRACE = shutil.which("strace")
FAILS = []
GREEN = "\x1b[38;2;152;195;121m"
# two rows, like the shipped default prompt: truecolour escapes and box
# glyphs above, and a last row -- readline's -- with a colour and a
# multibyte glyph of its own
PS1 = ("\\[\x1b[38;2;122;162;247m\\]\u256d\u2500\u2500 row\\[\x1b[0m\\]\\n"
       "\\[" + GREEN + "\\]\u276f \\[\x1b[0m\\]")
MARK = "\u276f ".encode()
# a whole CSI sequence, and a whole OSC/charset/two-byte ESC sequence
ESC_SEQ = re.compile(rb"\x1b(?:\[[0-9;?<=>!]*[ -/]*[@-~]|[()][0-9A-Za-z]"
                     rb"|\][^\x07\x1b]*(?:\x07|\x1b\\)|[ -~])")
WRITE = re.compile(r'^(\d+)\s+write\(2, "((?:\\x[0-9a-f]{2})*)", \d+\)\s+=\s+(\d+)')


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def utf8_len(b):
    if b >= 0xf0:
        return 4
    if b >= 0xe0:
        return 3
    if b >= 0xc0:
        return 2
    return 1


def atoms(stream):
    """(start, end) of every escape sequence and multibyte character."""
    out = []
    i = 0
    while i < len(stream):
        if stream[i] == 0x1b:
            m = ESC_SEQ.match(stream, i)
            n = (m.end() - i) if m else 1
        else:
            n = utf8_len(stream[i])
        if n > 1:
            out.append((i, i + n))
        i += n
    return out


def split_atoms(writes):
    """The atoms some write() boundary falls strictly inside."""
    stream = b"".join(writes)
    cuts = set()
    at = 0
    for w in writes[:-1]:
        at += len(w)
        cuts.add(at)
    bad = []
    for a, b in atoms(stream):
        if any(a < c < b for c in range(a + 1, b) if c in cuts):
            bad.append(stream[a:b])
    return bad


class Traced:
    def __init__(self, home, trace):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "PS1": PS1, "HELLISH_NO_BANNER": "1", "HELLISH_BANNER": "0",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        self.buf = bytearray()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(home)
            os.execve(STRACE, [STRACE, "-f", "-xx", "-s", "65535",
                               "-e", "trace=write", "-o", trace,
                               SHELL, "--norc"], env)
            os._exit(127)

    def drain(self, secs, quiet=0.3):
        end = time.monotonic() + secs
        last = time.monotonic()
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    return
                if not d:
                    return
                self.buf.extend(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet and self.buf:
                return

    def prompts(self):
        return self.buf.count(MARK)

    def wait_prompts(self, n, cap=15.0):
        end = time.monotonic() + cap
        while self.prompts() < n and time.monotonic() < end:
            self.drain(0.5, quiet=0.1)
        return self.prompts() >= n

    def send(self, data):
        os.write(self.fd, data)

    def close(self):
        try:
            os.write(self.fd, b"exit\r")
            self.drain(2.0)
        except OSError:
            pass
        try:
            os.kill(self.pid, 9)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass


def tty_writes(trace):
    """Every complete write() to fd 2, per pid, in order."""
    per = {}
    with open(trace, errors="replace") as f:
        for line in f:
            m = WRITE.match(line)
            if m:
                data = bytes.fromhex(m.group(2).replace("\\x", ""))
                per.setdefault(m.group(1), []).append(data[:int(m.group(3))])
    return per


def main():
    if not STRACE:
        print("skip: strace not installed (apt-get install strace)")
        return 0
    base = tempfile.mkdtemp(prefix="hellish_rowwrites_")
    home = os.path.join(base, "home")
    os.makedirs(home)
    trace = os.path.join(base, "trace")
    s = Traced(home, trace)
    check("the prompt appeared under strace", s.wait_prompts(1),
          repr(bytes(s.buf[-300:])))
    for i in range(4):
        s.send(b"\r")
        s.wait_prompts(2 + i)
    s.send(b"echo \xe2\x94\x80ok")
    s.drain(1.5)
    s.send(b"\x0c")             # ^L: readline redraws the whole prompt
    s.drain(1.5)
    s.send(b"\r")
    ok = s.wait_prompts(7)
    s.close()
    check("every prompt was drawn (5 Enters, a line, ^L)", ok,
          "saw %d" % s.prompts())
    writes = max(tty_writes(trace).values(), key=len, default=[])
    stream = b"".join(writes)
    rows = stream.count(GREEN.encode())
    check("the trace holds the prompt's last row", rows >= 6,
          "found it %d times in %d writes" % (rows, len(writes)))
    whole = sum(1 for w in writes if GREEN.encode() + MARK in w)
    check("readline's row reaches the tty in one write() each time",
          whole >= rows > 0,
          "%d of %d rows were whole inside a single write()" % (whole, rows))
    bad = split_atoms(writes)
    check("no escape sequence or UTF-8 character is split across write()s",
          not bad, "%d split, first: %r" % (len(bad), bad[:4]))
    shutil.rmtree(base, ignore_errors=True)
    print("%d failed (%d writes)" % (len(FAILS), len(writes)))
    return 1 if FAILS else 0


sys.exit(main())

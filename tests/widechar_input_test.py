#!/usr/bin/env python3
"""Typed non-ASCII input must stay in sync with what is on screen.

tests/widechar_prompt_test.py guards the PROMPT's handling of non-ASCII (the
cwd). Nothing guarded the other half -- the line the user types -- which is
what issue #2 reports: a line long enough to wrap, starting with a character
that is two bytes but one column, and an edit made at the start of it.

Honest scope: this is a GUARD, not a before/after regression. #2 has never
reproduced here (see the issue thread); the leading hypothesis is that the
reporter's terminal renders the prompt's ambiguous-width glyphs (`❯ ✦ ✘`) as
two columns while wcwidth calls them one, which no standard terminal model
can show. What this file does do is fail if hellish itself ever starts
miscounting typed multibyte input -- and it pins the shapes from the report
so a future readline or prompt change cannot silently break them.

Drives the real binary on a real pty, renders the output through a terminal
model, and compares the rendered line against the text that was typed.

    python3 tests/widechar_input_test.py build/bin/hellish
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

try:
    import pyte
except ImportError:
    print("SKIP widechar input (python3 -m pip install pyte)")
    sys.exit(0)

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
ROWS = 24
# the two lines from the report: same shape, one starts with n-tilde
ASCII = ("fasfsafasfsdcsyugcyuwgegfgsgcsuycgywggfwucyussgacyuwugfuyewuc"
         "uyweguagyfayugwucygguwcgy")
WIDE = ("ñfklsjafksajdflsajfskdjcosajofjosdjfoasfjasjfoisjoafjjsdfdij"
        "sdfoijisfsdfoaso")
MARK = "❯ "
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else " " + detail))
    if not ok:
        FAILS.append(name)


def session(payload, cols):
    """Feed payload to a pty-hosted shell; return the rendered screen."""
    home = tempfile.mkdtemp(prefix="hellish_wide_")
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
        # This test is about the RICH prompt's ambiguous-width glyphs
        # (❯ ✦ ✘); since the default became the basic zsh prompt, the
        # theme under test has to be asked for by name.
        "PS1": "\\B",
        "PS2": "> ", "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
        "HELLISH_NO_UPDATE_CHECK": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
    open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ,
                struct.pack("HHHH", ROWS, cols, 0, 0))
    time.sleep(0.7)
    screen = pyte.Screen(cols, ROWS)
    stream = pyte.ByteStream(screen)
    for chunk, pause in payload:
        os.write(fd, chunk)
        time.sleep(pause)
        while True:
            r, _, _ = select.select([fd], [], [], 0.15)
            if not r:
                break
            try:
                data = os.read(fd, 65536)
            except OSError:
                break
            if not data:
                break
            stream.feed(data)
    try:
        os.write(fd, b"\x03exit\n")
        time.sleep(0.3)
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except OSError:
        pass
    shutil.rmtree(home, ignore_errors=True)
    return "\n".join(l.rstrip() for l in screen.display)


def rendered_line(display):
    """The input line as the terminal drew it, wrap joins removed."""
    flat = display.replace("\n", "")
    i = flat.rfind(MARK)
    if i < 0:
        return flat.strip()
    return flat[i + len(MARK):].strip()


def main():
    # trims move the wrap boundary through the middle of the typed text
    for cols in (60, 100):
        for name, text in (("ascii", ASCII), ("wide", WIDE)):
            for trim in (0, 3, 8):
                typed = text[:len(text) - trim] if trim else text
                got = rendered_line(session(
                    [(typed.encode(), 0.6)], cols))
                check("%d cols, %s, trim %d: typed line renders intact"
                      % (cols, name, trim), got == typed,
                      "want %r got %r" % (typed, got))
                # the edit from the report: home, then insert
                got = rendered_line(session(
                    [(typed.encode(), 0.5), (b"\x01", 0.25),
                     (b"hello", 0.5)], cols))
                check("%d cols, %s, trim %d: C-a insert does not overwrite"
                      % (cols, name, trim), got == "hello" + typed,
                      "want %r got %r" % ("hello" + typed, got))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

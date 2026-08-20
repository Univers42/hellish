#!/usr/bin/env python3
"""Canary: the rendered prompt must never reach the terminal malformed.

This is a CANARY, not a regression test for a specific fix -- it passes on
code both with and without tty_write_all. It exists because prompt
corruption has been reported in the field and is hard to catch: an escape
sequence that arrives without its ESC[ introducer prints its body as
literal text (`8;2;90;96;106m`), and a UTF-8 character cut mid-sequence
arrives as U+FFFD. Both are silent -- nothing errors, the screen is just
wrong.

It drives the shell the way the reports describe (wide terminal so the
prompt frame is large and full of truecolor escapes, a reader that lags so
the tty output queue backs up, background jobs, repeated `clear`) and
asserts the transcript contains neither symptom.

Known limitation, stated so nobody trusts it further than it goes: this
harness has NOT reproduced the field corruption. Roughly 1MB of captured
output across several stress shapes came back clean. So a pass here means
"the common paths are intact", not "the reported bug is gone".

Usage: python3 prompt_shortwrite_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import struct
import sys
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []

# An SGR body that reached the screen without its ESC[ introducer: digits and
# semicolons ending in 'm', sitting next to prompt text rather than after ESC.
ORPHAN_SGR = re.compile(r'(?<!\x1b\[)(?<![\x1b\[0-9;])\b\d{1,3}(?:;\d{1,3}){2,}m')


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(cols=200, rounds=14):
    """Drive the shell with a laggy reader so the tty output queue fills."""
    env = {
        "HOME": os.environ.get("HOME", "/tmp"),
        "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8",
        "COLORTERM": "truecolor",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    # A wide terminal makes the prompt's box-drawing line long, so one frame
    # is several hundred bytes of escapes and multibyte glyphs.
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 50, cols, 0, 0))
    raw = b""
    time.sleep(0.7)

    def feed(data, settle, lag=0.0):
        nonlocal raw
        os.write(fd, data)
        end = time.time() + settle
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                if lag:
                    time.sleep(lag)      # let the tty queue back up
                try:
                    raw += os.read(fd, 4096)
                except OSError:
                    return

    for _ in range(rounds):
        # clear floods the output queue; the background job then lands a
        # SIGCHLD while the next prompt frame is being written.
        feed(b"clear\n", 0.35, lag=0.02)
        feed(b"sleep 0.05 &\n", 0.30, lag=0.02)
        feed(b"cd /sgoinfre/students/dlesieur/hellish\n", 0.30, lag=0.02)
        feed(b"cd vendor/libft\n", 0.30, lag=0.02)
        feed(b"cd -\n", 0.30, lag=0.02)
    feed(b"", 0.8)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return raw


def main():
    raw = run()
    text = raw.decode("utf-8", errors="replace")

    check("shell produced output", len(raw) > 2000, "got %d bytes" % len(raw))

    bad = ORPHAN_SGR.findall(text)
    check("no escape sequence was cut by a short write",
          not bad, "orphaned SGR bodies on screen: %s" % bad[:5])

    check("no UTF-8 character was cut by a short write",
          "�" not in text,
          "replacement char at %s" % text.find("�"))

    # A frame cut mid-escape also tends to leave a bare ESC[ with nothing
    # after it before ordinary text.
    check("no truncated CSI introducer",
          not re.search(r'\x1b\[(?![\d;?]*[ -~])', text))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

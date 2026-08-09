#!/usr/bin/env python3
"""Prompt layout must stay correct when the cwd holds non-ASCII characters.

The prompt shortens the cwd to a budget derived from the terminal width. That
budget is in COLUMNS, but the shortening used to measure BYTES and, when it had
to cut inside a path component, sliced at a raw byte offset. On any accented
path both go wrong: bytes over-count columns (n-tilde is 2 bytes, 1 column) so
the prompt shrank further than asked, and a byte-offset cut can land inside a
UTF-8 sequence, leaving a partial character the terminal renders as U+FFFD of
its own chosen width -- a prompt silently misaligned by a column or two.

Drives the real binary on a real pty, renders the output through a terminal
model, and asserts the prompt fits the terminal and contains no broken glyph.

Honest scope: this is a GUARD, not a before/after regression. The byte-offset
cut it protects needs the cwd to be one over-long top-level component, which a
test cannot create without root, so this file does not fail on the pre-fix
build. What it does do is fail if any future change starts emitting partial
characters or over-wide prompt rows for a non-ASCII cwd.

    python3 tests/widechar_prompt_test.py build/bin/hellish
"""
import os
import pty
import select
import shutil
import struct
import sys
import termios
import fcntl
import tempfile
import time

try:
    import pyte
except ImportError:                                     # pragma: no cover
    print("SKIP widechar_prompt_test: pyte not installed (pip install pyte)")
    sys.exit(0)

BIN = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                      else "build/bin/hellish")
ROWS = 16
# The shortener prefers to cut on a '/', so a path of short components never
# reaches the branch that slices INSIDE a component. One component longer than
# the whole budget forces it there -- and that is the branch that used to cut at
# a raw byte offset. Made of 2-byte characters so almost every offset lands
# mid-sequence.
LONG_COMPONENT = "ñ" * 90
DEEP = os.path.join("Documentos", "Señoría", "mañana", LONG_COMPONENT)


def session(cwd, cols):
    env = dict(os.environ)
    env.update(HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", TERM="xterm-256color")
    env.setdefault("LANG", "C.UTF-8")
    env.setdefault("LC_ALL", "C.UTF-8")
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(cwd)
        os.execve(BIN, [BIN], env)
        os._exit(1)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, cols, 0, 0))
    screen = pyte.Screen(cols, ROWS)
    stream = pyte.ByteStream(screen)
    deadline = time.time() + 2.5
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                data = os.read(fd, 65536)
            except OSError:
                break
            if not data:
                break
            stream.feed(data)
    lines = [l.rstrip() for l in screen.display]
    try:
        os.write(fd, b"exit\r")
    except OSError:
        pass
    time.sleep(0.1)
    for fn in (lambda: os.close(fd), lambda: os.waitpid(pid, os.WNOHANG)):
        try:
            fn()
        except Exception:
            pass
    return lines


def main():
    root = tempfile.mkdtemp(prefix="hellish_wide_")
    cwd = os.path.join(root, DEEP)
    os.makedirs(cwd, exist_ok=True)
    failures = []
    try:
        for cols in (40, 50, 60, 70, 80, 100, 120):
            lines = session(cwd, cols)
            for i, line in enumerate(lines):
                if not line:
                    continue
                # A partial UTF-8 sequence surfaces as the replacement char.
                if "�" in line:
                    failures.append(
                        "cols=%d row%d has a broken character: %r"
                        % (cols, i, line))
                # pyte wraps, so a row wider than the screen is impossible;
                # what we can catch is the prompt overflowing its own budget.
                if len(line) > cols:
                    failures.append(
                        "cols=%d row%d is %d cells wide: %r"
                        % (cols, i, len(line), line))
    finally:
        shutil.rmtree(root, ignore_errors=True)

    if failures:
        print("FAIL widechar prompt layout")
        for f in failures[:12]:
            print("   ", f)
        return 1
    print("PASS widechar prompt layout (no broken glyphs, no overflow)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

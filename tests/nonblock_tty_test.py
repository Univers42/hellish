#!/usr/bin/env python3
"""Regression test: the prompt survives a NON-BLOCKING terminal (issue #34).

#34 reported intermittent prompt corruption -- an SGR body printed as
literal text (`8;2;90;96;106m`, the tail of ESC[38;2;90;96;106m arriving
without its introducer) or a lone U+FFFD where `❯` should be. Both are
the same fault at two cut points: a chunk of the frame went missing
between the shell and the terminal.

The first repair made every prompt writer go through tty_write_all, which
loops over short writes. That closed one hole and left a wider one: the
loop retried EINTR but returned on any other error, and EAGAIN is an
error. O_NONBLOCK lives on the open file DESCRIPTION, which the shell
shares with every program it launches, so a single tool that sets it on
the terminal and does not restore it leaves the shell writing to a
non-blocking tty for the rest of the session. With the output queue full,
write() returns -1/EAGAIN having transferred nothing -- so the whole
frame vanished, not merely its tail.

Measured directly on a full 64K buffer before the fix:

    EINTR-only loop   wrote  0 / 65 bytes of a prompt frame
    EAGAIN wait       wrote 65 / 65

Checks here drive the real shell through a pty whose slave is put into
non-blocking mode, with a reader that deliberately lags, and assert that
what arrives is never a truncated escape or a broken UTF-8 glyph.

Usage: python3 nonblock_tty_test.py /path/to/hellish
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

ENV = {
    "HOME": os.environ.get("HOME", "/tmp"),
    "PATH": os.environ["PATH"],
    "TERM": "xterm-256color", "LANG": "C.UTF-8",
    "COLORTERM": "truecolor",
    "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
    "ASAN_OPTIONS": "detect_leaks=0",
}


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def run_nonblocking(cmds, slow_reader):
    """Drive the shell on a pty whose SLAVE is O_NONBLOCK.

    The shell inherits the slave as stdout, so every prompt frame it
    writes hits the non-blocking path. `slow_reader` throttles how fast
    we drain the master, which is what lets the output queue fill.
    """
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(ENV)
        flags = fcntl.fcntl(1, fcntl.F_GETFL)
        fcntl.fcntl(1, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 200, 0, 0))
    time.sleep(0.8)
    out = b""
    for c in cmds:
        os.write(fd, c)
        end = time.time() + 0.9
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try:
                    out += os.read(fd, 512 if slow_reader else 65536)
                except OSError:
                    break
                if slow_reader:
                    time.sleep(0.02)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    os.close(fd)
    return out


# An SGR body that reached the screen without its ESC[ introducer is the
# exact signature from the report: digits and semicolons ending in 'm',
# sitting outside any escape sequence.
ORPHAN_SGR = re.compile(rb"(?<!\x1b\[)(?<![0-9;])[0-9]+(?:;[0-9]+){2,}m")


def strip_escapes(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b)


def run_stalled(cmds, stall_bytes=400000):
    """Fill the tty output queue, THEN let the shell draw a prompt.

    This is the check with teeth. We stop reading the master entirely and
    make the shell emit far more than the ~64K the line discipline will
    hold, so its next write really does hit EAGAIN on the non-blocking
    slave. Only then do we drain and look at what survived.
    """
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(ENV)
        flags = fcntl.fcntl(1, fcntl.F_GETFL)
        fcntl.fcntl(1, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 200, 0, 0))
    time.sleep(0.8)
    # flood, reading nothing at all -- the queue backs up and stays full
    os.write(fd, b"yes ABCDEFGHIJKLMNOPQRSTUVWXYZ | head -c %d\n"
             % stall_bytes)
    time.sleep(2.0)
    for c in cmds:
        os.write(fd, c)
        time.sleep(0.4)
    # now drain everything
    out = b""
    end = time.time() + 6.0
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.2)
        if not r:
            break
        try:
            chunk = os.read(fd, 65536)
        except OSError:
            break
        if not chunk:
            break
        out += chunk
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    os.close(fd)
    return out


def main():
    cmds = [b"cd /tmp\n", b"cd -\n", b"clear\n", b"echo MARK1\n",
            b"cd /usr\n", b"cd -\n", b"echo MARK2\n"]

    for label, slow in (("fast reader", False), ("lagging reader", True)):
        out = run_nonblocking(cmds, slow)
        check("non-blocking tty, %s: shell still responds" % label,
              b"MARK1" in out and b"MARK2" in out,
              "got %r" % out[-200:])

        body = strip_escapes(out)
        orphan = ORPHAN_SGR.search(body)
        check("non-blocking tty, %s: no orphaned SGR body" % label,
              orphan is None,
              "found %r" % (orphan.group(0) if orphan else b""))

        check("non-blocking tty, %s: no U+FFFD from a split glyph" % label,
              b"\xef\xbf\xbd" not in out and "�" not in
              out.decode("utf-8", errors="replace"),
              "replacement char present")

        # A frame cut mid-escape leaves a bare ESC with no final byte.
        dangling = re.search(rb"\x1b\[[0-9;?]*$", out)
        check("non-blocking tty, %s: no truncated escape at the tail" % label,
              dangling is None, "tail=%r" % out[-40:])

    # The check with teeth: the output queue is genuinely full when the
    # shell next draws, so write() really returns EAGAIN. Without the
    # EAGAIN wait the shell drops whole frames here and the markers after
    # the flood never arrive at all.
    out = run_stalled([b"echo STALL1\n", b"cd /tmp\n", b"cd -\n",
                       b"echo STALL2\n"])
    check("queue-full tty: shell keeps writing after EAGAIN",
          b"STALL1" in out and b"STALL2" in out,
          "markers lost -- frames dropped on EAGAIN; tail=%r" % out[-200:])

    body = strip_escapes(out)
    orphan = ORPHAN_SGR.search(body)
    check("queue-full tty: no orphaned SGR body",
          orphan is None, "found %r" % (orphan.group(0) if orphan else b""))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

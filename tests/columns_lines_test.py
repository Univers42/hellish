#!/usr/bin/env python3
"""COLUMNS and LINES in interactive shells (issue #97).

bash sets both variables at interactive startup and keeps them current
across terminal resizes (checkwinsize, default-on since bash 5.0); they
are shell variables, NOT exported. hellish set neither, so every prompt
that right-aligns or wraps silently fell back to 80 columns — the only
workaround was caching `tput cols` from a WINCH trap.

Asserted here, each in a pty with an explicit TIOCSWINSZ size:
  1. startup:   a 107x31 pty reports COLUMNS=107 LINES=31
  2. resize:    shrinking the pty to 91x24 mid-session is visible to the
                next command (the kernel raises SIGWINCH; hellish must
                refresh no later than the next execution)
  3. unexported: `env` inside the session shows no COLUMNS= line — a
                child process must measure its own terminal (bash parity)
  4. non-interactive: `hellish -c` sets neither variable

Usage: python3 columns_lines_test.py [/path/to/hellish]
"""
import fcntl
import os
import pty
import re
import select
import struct
import subprocess
import sys
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def clean_env():
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_BANNER="0",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1",
               TERM="xterm")
    for v in ("COLUMNS", "LINES", "PS1", "PROMPT"):
        env.pop(v, None)
    return env


def set_winsz(fd, cols, rows):
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols,
                                                    0, 0))


def drain(fd, deadline):
    out = b""
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.2)
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            break
        if not d:
            break
        out += d
    return out


def interactive_session():
    """One pty session: sized 107x31 before exec, resized to 91x24
    mid-session. Returns the full decoded transcript. The env is built
    BEFORE the fork: a near-empty environment is a different test."""
    env = clean_env()
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL, "--norc"], env)
        os._exit(1)
    set_winsz(fd, 107, 31)
    time.sleep(0.6)
    os.write(fd, b'echo "CL1=[$COLUMNS/$LINES]"\r')
    time.sleep(0.5)
    os.write(fd, b"env | grep -c ^COLUMNS=\r")
    time.sleep(0.5)
    set_winsz(fd, 91, 24)
    time.sleep(0.3)
    os.write(fd, b'echo "CL2=[$COLUMNS/$LINES]"\r')
    time.sleep(0.5)
    os.write(fd, b"exit\r")
    out = drain(fd, time.time() + 5)
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    return out.decode("utf-8", "replace")


def main():
    text = interactive_session()
    plain = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", text)
    lines = [ln.strip() for ln in plain.replace("\r", "\n").splitlines()]
    check("startup: COLUMNS/LINES from the pty size",
          "CL1=[107/31]" in text, repr(text[-500:]))
    check("unexported: no COLUMNS= in child env",
          "0" in lines and "1" not in lines, repr(text[-500:]))
    check("resize: next command sees 91/24",
          "CL2=[91/24]" in text, repr(text[-500:]))

    p = subprocess.run([SHELL, "-c", 'echo "NI=[${COLUMNS:-unset}]"'],
                       capture_output=True, text=True, env=clean_env(),
                       timeout=30)
    check("non-interactive: COLUMNS stays unset",
          p.stdout.strip() == "NI=[unset]", repr(p.stdout))

    if FAILS:
        print("\n%d FAILED: %s" % (len(FAILS), ", ".join(FAILS)))
        sys.exit(1)
    print("\nall clear")


main()

#!/usr/bin/env python3
"""Regression test: a pending update stays visible in the prompt.

The loud "update available" notice is emitted once, between commands, and
then `notified` is set so it is never shown again -- deliberately, because
a banner on every prompt is how users learn to stop reading banners. But
"once, ever" meant a user who was scrolled away when it printed, or who
closed that terminal, never found out at all: the background check only
runs daily, so nothing brought it back. Reported from the field exactly
that way -- a new session proposed nothing while `update` itself found the
release immediately.

The badge is the quiet half of that design: it persists for as long as the
update is actually pending, and disappears by itself once the new binary
is in place.

Checks:
  1. built-in prompt shows "⬆<version>" when the state file has a newer
     release -- even when `notified` is already set, which is the case
     that was broken.
  2. nothing is shown when the cached version is the running one.
  3. HELLISH_NO_UPDATE_CHECK suppresses it, like every other part of the
     update subsystem.
  4. the \\U escape carries it into a custom PS1, and is self-spacing:
     it renders nothing at all when no update is pending.

Usage: python3 update_badge_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
HERE = os.path.dirname(os.path.abspath(__file__))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def current_version():
    """The version baked into the binary under test."""
    out = subprocess.run([SHELL, "--version"], capture_output=True, text=True,
                         env=dict(os.environ, HELLISH_NO_BANNER="1")).stdout
    m = re.search(r"version ([0-9]+\.[0-9]+\.[0-9]+)", out)
    return m.group(1) if m else "0.0.0"


def state(home, latest, notified=1):
    d = os.path.join(home, ".cache", "hellish")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "state"), "w") as f:
        f.write("latest=%s\nchecked=%d\nnotified=%d\n"
                "header_shown=%d\nheader_rev=99\nheader_ver=%s\n"
                % (latest, int(time.time()), notified,
                   int(time.time()), current_version()))


def render(home, env_extra=None):
    """Start the shell on a pty and return what it painted."""
    env = {
        "HOME": home, "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "COLORTERM": "truecolor",
        "HELLISH_NO_BANNER": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    if env_extra:
        env.update(env_extra)
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.chdir("/tmp")
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 140, 0, 0))
    raw = b""
    time.sleep(1.0)
    end = time.time() + 1.2
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                raw += os.read(fd, 65536)
            except OSError:
                break
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return raw.decode("utf-8", errors="replace")


def main():
    home = tempfile.mkdtemp(prefix="hellish_badge_")
    cur = current_version()

    # 1: pending update, already announced -> the badge still shows it
    state(home, "9.9.9", notified=int(time.time()))
    out = render(home)
    check("badge shows a pending update the notice already announced",
          "⬆9.9.9" in out, "prompt had no badge")

    # 3: the global opt-out wins
    out = render(home, {"HELLISH_NO_UPDATE_CHECK": "1"})
    check("HELLISH_NO_UPDATE_CHECK suppresses the badge",
          "⬆" not in out)

    # 4: \U carries it into a custom PS1
    out = render(home, {"PS1": "[\\U] $ "})
    check("\\U shows the badge in a custom PS1", "⬆9.9.9" in out)

    # 2: nothing pending -> nothing shown, anywhere
    state(home, cur, notified=0)
    out = render(home)
    check("no badge when the cached version is the running one",
          "⬆" not in out, "badge shown with latest==%s" % cur)
    out = render(home, {"PS1": "[\\U] $ "})
    check("\\U is self-spacing: renders nothing with no update",
          "⬆" not in out)

    shutil.rmtree(home, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""The interactive half of issue #20: the header gate and the notice.

Both need a real terminal, so this drives the shell on a pty.

  9  header  shown the first time, quiet on a second run the same day,
              shown again when the header revision or the version changes
  3  typing  a discovered update must not disturb a line being typed:
              the notice waits for the next prompt, the typed line survives
              intact and still runs

Usage: python3 tests/update_ui_test.py [path/to/hellish]
"""
import fcntl
import os
import pty
import re
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

ROOT = os.path.dirname(os.path.abspath(__file__))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "..", "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def session(home, script, extra=None, settle=1.0):
    """Run `script` (bytes) through a pty-hosted interactive shell."""
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "> ",
           "HELLISH_NO_ANIM": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "ASAN_OPTIONS": "detect_leaks=0"}
    if extra:
        env.update(extra)
    # a key set to "" still counts as set to getenv(), so an opt-out is
    # removed rather than blanked
    for k in [k for k, v in env.items() if v == ""]:
        del env[k]
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 120, 0, 0))
    time.sleep(0.8)
    out = b""
    for chunk, pause in script:
        os.write(fd, chunk)
        time.sleep(pause)
        while True:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                break
            try:
                b = os.read(fd, 65536)
            except OSError:
                b = b""
            if not b:
                break
            out += b
    time.sleep(settle)
    while True:
        r, _, _ = select.select([fd], [], [], 0.2)
        if not r:
            break
        try:
            b = os.read(fd, 65536)
        except OSError:
            break
        if not b:
            break
        out += b
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except OSError:
        pass
    return re.sub(rb"\x1b\[[0-9;?]*[ -/]*[@-~]", b"", out).decode(
        "utf-8", "replace")


def state_path(home):
    return os.path.join(home, ".cache", "hellish", "state")


def poke_state(home, **kw):
    os.makedirs(os.path.dirname(state_path(home)), exist_ok=True)
    cur = {}
    if os.path.exists(state_path(home)):
        for line in open(state_path(home)):
            if "=" in line:
                k, v = line.rstrip("\n").split("=", 1)
                cur[k] = v
    cur.update({k: str(v) for k, v in kw.items()})
    with open(state_path(home), "w") as f:
        for k, v in cur.items():
            f.write("%s=%s\n" % (k, v))


def main():
    home = tempfile.mkdtemp(prefix="hellish_ui_")

    # 9 -- the header is news, not furniture
    out = session(home, [(b"exit\n", 0.4)])
    check("header: shown on a first-ever run", "hellish" in out,
          repr(out[:160]))
    out = session(home, [(b"exit\n", 0.4)])
    check("header: quiet on a second run the same day",
          "Type `help'" not in out and out.count("hellish") <= 1,
          repr(out[:200]))
    poke_state(home, header_rev=999)
    out = session(home, [(b"exit\n", 0.4)])
    check("header: back when its revision changes", "hellish" in out,
          repr(out[:160]))
    poke_state(home, header_ver="0.0.0")
    out = session(home, [(b"exit\n", 0.4)])
    check("header: back after the shell is upgraded", "hellish" in out,
          repr(out[:160]))
    yesterday = int(time.time()) - 86400 * 2
    poke_state(home, header_shown=yesterday)
    out = session(home, [(b"exit\n", 0.4)])
    check("header: eligible again the next day", "hellish" in out,
          repr(out[:160]))

    # 3 -- a pending update must not touch a line being typed
    poke_state(home, latest="99.0.0", checked=int(time.time()), notified=0,
               header_shown=int(time.time()), header_rev=1)
    typed = "echo THE-USER-WAS-TYPING-THIS"
    out = session(home, [(typed.encode(), 1.2), (b"\n", 0.6),
                         (b"exit\n", 0.4)],
                  extra={"HELLISH_NO_UPDATE_CHECK": ""})
    check("typing: the notice appears", "update available" in out,
          repr(out[-400:]))
    check("typing: the typed line survives intact",
          typed in out, repr(out[-400:]))
    check("typing: the typed line still runs",
          "THE-USER-WAS-TYPING-THIS" in out.split(typed, 1)[-1],
          repr(out[-400:]))
    check("typing: nothing else was executed",
          "command not found" not in out, repr(out[-400:]))

    # and it is said once, not on every prompt
    out = session(home, [(b"true\n", 0.4), (b"true\n", 0.4), (b"exit\n", 0.4)],
                  extra={"HELLISH_NO_UPDATE_CHECK": ""})
    check("notice: said once, not on every prompt",
          out.count("update available") == 0, repr(out[-300:]))

    shutil.rmtree(home, ignore_errors=True)
    print("\n%d check(s) failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

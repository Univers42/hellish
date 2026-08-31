#!/usr/bin/env python3
"""Regression test: a virtualenv prompt survives its own deactivate.

issue #39. Python's venv activate does

    _OLD_VIRTUAL_PS1="${PS1:-}"
    PS1="(venv) ${PS1:-}"

and deactivate puts it back only `if [ -n "$_OLD_VIRTUAL_PS1" ]`. hellish
shipped no default PS1, so that saved value was the empty string, the
guard was false, and PS1 stayed "(venv) " for the rest of the session --
the reporter's prompt said (testenv) long after they had left it, and the
built-in prompt never came back either, because a non-empty PS1 replaces
it entirely.

The fix is that interactive shells now start with PS1='\\B', an escape
that renders exactly what an unset PS1 used to render. The default look is
unchanged; what changed is that PS1 has a value to save and restore.

Checked here at two levels:
  1. The PS1 handshake itself, run verbatim as commands -- this is the
     mechanism, independent of whether python3 is installed.
  2. The real thing: python3 -m venv, activate, deactivate. Skipped when
     python3 or the venv module is unavailable.

Usage: python3 venv_prompt_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(seq, home=None):
    env = {
        "HOME": home or os.environ.get("HOME", "/tmp"),
        "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        # --norc: pin the config. An inherited ~/.hellishrc can set PS1 or
        # define names, and quietly decide what this test sees.
        os.execvp(SHELL, [SHELL, "--norc"])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    time.sleep(0.9)
    out = b""
    for data, wait in seq:
        try:
            os.write(fd, data)
        except OSError:
            break
        end = time.time() + wait
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return out.decode(errors="replace")


def after_last(text, marker):
    """Everything printed after the LAST occurrence of marker."""
    i = text.rfind(marker)
    if i < 0:
        return ""
    return text[i + len(marker):]


def main():
    # 1: the PS1 handshake, exactly as venv performs it.
    seq = [
        (b'_OLD_VIRTUAL_PS1="${PS1:-}"\n', 1.0),
        (b'PS1="(testenv) ${PS1:-}"\n', 1.2),
        (b"echo AAA_ACTIVE\n", 1.2),
        (b'if [ -n "${_OLD_VIRTUAL_PS1:-}" ] ; then PS1="${_OLD_VIRTUAL_PS1:-}" ; fi\n', 1.2),
        (b"echo ZZZ_DONE\n", 1.4),
    ]
    out = run(seq)
    check("PS1 is non-empty by default (venv can save and restore it)",
          "(testenv)" in out,
          "activate's prefix never appeared at all: %r" % out[-200:])
    active = after_last(out, "AAA_ACTIVE")
    check("the venv name shows while the venv is active",
          "(testenv)" in active, "tail=%r" % active[:200])
    done = after_last(out, "ZZZ_DONE")
    check("the venv name is GONE after deactivate restores PS1",
          "(testenv)" not in done, "prompt still says it: %r" % done[:200])
    # The restored default is zsh's basic "hostname% " -- the rich theme
    # stopped being the default, so what must come back is the hostname.
    check("the default prompt comes back after deactivate",
          socket.gethostname().split(".")[0] in done,
          "no default prompt after restore: %r" % done[:200])

    # 2: the real thing, if python3 can build a venv here.
    probe = subprocess.run([sys.executable, "-c", "import venv"],
                           capture_output=True)
    if probe.returncode != 0:
        print("skip  real python venv (venv module unavailable)")
    else:
        d = tempfile.mkdtemp(prefix="hellish_venv_")
        try:
            seq = [(b"cd " + d.encode() + b"\n", 1.0),
                   (b"python3 -m venv tenv\n", 20.0),
                   (b". tenv/bin/activate\n", 2.0),
                   (b"echo AAA_ACTIVE\n", 1.2),
                   (b"deactivate\n", 2.0),
                   (b"echo ZZZ_DONE\n", 1.6)]
            out = run(seq, home=d)
            active = after_last(out, "AAA_ACTIVE")
            done = after_last(out, "ZZZ_DONE")
            check("real venv: name shows while active",
                  "(tenv)" in active, "tail=%r" % active[:200])
            # The shell has to still BE there. Without this, a shell that
            # died inside deactivate prints no prompt at all, and "the name
            # is gone" passes for entirely the wrong reason.
            check("real venv: the shell survives deactivate",
                  out.count("ZZZ_DONE") >= 2,
                  "shell did not reach the marker; tail=%r" % out[-300:])
            check("real venv: name is gone after deactivate",
                  "(tenv)" not in done and done.strip() != "",
                  "tail=%r" % done[:200])
        finally:
            shutil.rmtree(d, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

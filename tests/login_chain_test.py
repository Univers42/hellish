#!/usr/bin/env python3
"""Regression test: a login shell's startup chain -- issue #51.

The report is `make my_shell` on Ubuntu 24: chsh to hellish, log in, and
the session opens with errors and a PATH the user did not trust, which is
why they ended up hand-rolling

    case ":$PATH:" in
        *":$HOME/.local/bin:"*) ;;
        *) export PATH="$HOME/.local/bin:$PATH" ;;
    esac

into their rc three times over. They were right that it should not be
necessary: a LOGIN hellish sources /etc/profile and then ~/.profile, and on
Debian/Ubuntu ~/.profile is exactly what puts ~/.local/bin and ~/bin on
PATH. The chain was working; the noise around it made it look like it was
not.

The noise was two shopt probes in stock dotfiles (see
shopt_setopt_test.py for the unit-level pinning of both):

    /etc/profile.d/bash_completion.sh:  shopt -q progcomp
    ~/.bashrc (Ubuntu skel):            if ! shopt -oq posix; then

which printed "shopt: progcomp: invalid shell option name" and then dumped
all 27 set -o options onto the screen, at every single login.

This test runs the real thing: a login hellish on a pty, against a
throwaway HOME holding a ~/.profile built from Ubuntu's stock snippets,
including both probes verbatim. The probes live in the sandbox rather than
being read from the host's /etc, so the test says the same thing on a
developer's machine and on a CI runner that has neither file.

Usage: python3 login_chain_test.py /path/to/hellish
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

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "build/bin/hellish")
FAILS = []
ESC_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07")

# Ubuntu's stock startup logic, trimmed to the parts that touch this bug.
# `. ~/.bashrc` is guarded on BASH_VERSION, which hellish advertises, so
# hellish takes that branch exactly like bash does -- that is how the
# ~/.bashrc probe reaches it at all.
PROFILE = """\
if [ -n "$BASH_VERSION" ]; then
    if [ -f "$HOME/.bashrc" ]; then
        . "$HOME/.bashrc"
    fi
fi

# /etc/profile.d/bash_completion.sh asks this on every login.
if shopt -q progcomp; then
    echo PROBE_PROGCOMP=on
else
    echo PROBE_PROGCOMP=off
fi

if [ -d "$HOME/bin" ] ; then
    PATH="$HOME/bin:$PATH"
fi

if [ -d "$HOME/.local/bin" ] ; then
    PATH="$HOME/.local/bin:$PATH"
fi
echo PROFILE_DONE
"""

# Ubuntu's skel ~/.bashrc, trimmed the same way.
BASHRC = """\
case $- in
    *i*) ;;
      *) return;;
esac
shopt -s histappend
shopt -s checkwinsize
if ! shopt -oq posix; then
    echo PROBE_POSIX=off
fi
echo BASHRC_DONE
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def login(home):
    """Drive `hellish --login` on a pty against a sandboxed HOME."""
    env = {
        "HOME": home,
        "PATH": "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "TERM": "xterm-256color", "LANG": "C.UTF-8",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "--login"])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 120, 0, 0))
    out = b""
    end = time.time() + 4.0          # let the whole chain settle first
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                out += os.read(fd, 65536)
            except OSError:
                break
    banner = len(out)
    for cmd in (b"echo PATHIS=$PATH\n", b"echo EDITORIS=$EDITOR\n"):
        os.write(fd, cmd)
        end = time.time() + 2.0
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
    text = ESC_RE.sub("", out.decode(errors="replace"))
    return text, ESC_RE.sub("", out[:banner].decode(errors="replace"))


def main():
    home = tempfile.mkdtemp()
    try:
        os.mkdir(os.path.join(home, "bin"))
        os.makedirs(os.path.join(home, ".local", "bin"))
        with open(os.path.join(home, ".profile"), "w") as f:
            f.write(PROFILE)
        with open(os.path.join(home, ".bashrc"), "w") as f:
            f.write(BASHRC)
        # Seed the rc the way an install does, so this also covers the
        # config a `make my_shell` user is now given (issue #51's first
        # line): EDITOR must actually arrive in the session.
        shutil.copy(os.path.join(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__))), "hellishrc.example"),
            os.path.join(home, ".hellishrc"))

        out, startup = login(home)

        # 1. The chain runs to the end. If ~/.profile aborts early its PATH
        #    lines never run, which is the failure the user was papering
        #    over by hand.
        check("~/.bashrc is sourced through ~/.profile",
              "BASHRC_DONE" in out, "got %r" % startup[:300])
        check("~/.profile runs to completion", "PROFILE_DONE" in out,
              "got %r" % startup[:300])

        # 2. Neither probe makes a mess of the login.
        check("no shopt error at login",
              "invalid shell option name" not in out
              and "invalid option name" not in out,
              "got %r" % startup[:300])
        check("shopt -oq posix does not dump the option table at login",
              "braceexpand" not in out and "interactive-comments" not in out,
              "the whole set -o table hit the screen: %r" % startup[:300])
        check("the ~/.bashrc posix branch is taken", "PROBE_POSIX=off" in out,
              "got %r" % startup[:300])
        # progcomp answers "off" until `shopt -s progcomp` asks for it, and
        # THIS PROBE IS WHY. It is the gate /etc/profile.d/bash_completion.sh
        # checks; answering "on" made the runner source a 3800-line framework
        # hellish cannot yet run, and the login opened with a syntax error --
        # #51's exact complaint, arriving through the door #51 opened. The
        # dispatch itself works (progcomp_test.py drives git's completion
        # through a real TAB); it is the DEFAULT that waits.
        check("progcomp answers instead of erroring",
              "PROBE_PROGCOMP=off" in out, "got %r" % startup[:300])
        check("the login chain prints nothing of its own",
              "syntax error" not in startup and "not found" not in startup,
              "a login shell said something: %r" % startup[:400])

        # 3. PATH: the login chain does the job, unaided. This is the claim
        #    the hand-rolled `case ":$PATH:"` block was compensating for.
        path = ""
        for line in out.splitlines():
            if line.startswith("PATHIS="):
                path = line[len("PATHIS="):]
        check("~/.profile's ~/.local/bin reaches PATH",
              os.path.join(home, ".local/bin") in path, "PATH=%r" % path)
        check("~/.profile's ~/bin reaches PATH",
              os.path.join(home, "bin") in path, "PATH=%r" % path)
        check("the system entries survive", "/usr/bin" in path,
              "PATH=%r" % path)
        check("PATH has no duplicate ~/.local/bin",
              path.split(":").count(os.path.join(home, ".local/bin")) == 1,
              "PATH=%r" % path)

        # 4. The seeded rc is sourced and its settings land.
        check("~/.hellishrc reaches the session (EDITOR is set)",
              "EDITORIS=vim" in out, "got %r" % out[-300:])
    finally:
        shutil.rmtree(home, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

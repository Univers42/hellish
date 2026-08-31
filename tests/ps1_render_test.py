#!/usr/bin/env python3
"""Regression test: PS1 rendering, and the login chain that exposed it.

issue #41. hellish sets BASH_VERSION, so Debian/Ubuntu's stock ~/.profile
takes its `if [ -n "$BASH_VERSION" ]` branch and sources ~/.bashrc -- which
sets bash's default PS1. Rendering that PS1 put this on the user's screen:

    :+()}\\033[01;32mdlesieur@dlesieur42\\033[00m:\\033[01;34m~/...

instead of a coloured prompt. Two independent holes in the renderer:

  1. \\nnn (octal) was not an escape, so the \\033 that opens every colour
     span fell through the unknown-escape path and printed as four
     characters. Every colour code in the prompt became visible text.

  2. ${...} was read as a bare name only. On ${debian_chroot:+(...)} the
     reader stopped at the ':' and let the operator's own text reach the
     screen -- the stray ":+()}" above. It now goes through the shell's
     real parameter expander, so every form the word expander knows works
     in a prompt too.

Also checks that `set -o <bad>` names the option it rejected, the way bash
does -- the report's "set: invalid option name" gave the user no way to
tell which option their rc file got wrong.

Usage: python3 ps1_render_test.py /path/to/hellish
"""
import fcntl
import atexit
import os
import pty
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []

# HOME is a scratch directory and the shell is started with --norc, so the
# prompt under test is the PS1 this file sets and nothing else.
#
# Without both, the test reads the DEVELOPER'S configuration: ~/.hellishrc
# and $XDG_CONFIG_HOME/hellish/rc.d/*.hsh are sourced for an interactive
# shell, and anything in them that sets PS1 wins over the inherited one. The
# symptom is every case failing with the developer's own prompt in the
# `got` field -- a verdict about their dotfiles reported as a verdict about
# the renderer. Same isolation completion_posix_test.py uses, for the same
# reason.
HOME_DIR = tempfile.mkdtemp(prefix="ps1_render_home_")
atexit.register(shutil.rmtree, HOME_DIR, True)

BASE_ENV = {
    "HOME": HOME_DIR,
    "PATH": os.environ["PATH"],
    "TERM": "xterm-256color", "LANG": "C.UTF-8",
    "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
    "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
}

# Debian/Ubuntu's stock interactive PS1, verbatim.
UBUNTU_PS1 = (r"${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h"
              r"\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ ")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def render(ps1, extra=None, send=b"echo MARKER\n", settle=1.6):
    """Start hellish on a pty with PS1 set; return everything it printed."""
    env = dict(BASE_ENV)
    env["PS1"] = ps1
    if extra:
        env.update(extra)
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        try:
            os.execv(SHELL, [SHELL, "--norc"])
        except BaseException:
            pass
        # Nothing may escape this block. An exception here unwinds back into
        # main() and runs the whole file again as a second process, turning
        # one clear error into a cascade of unrelated ones with a Python
        # traceback embedded in the pty capture.
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    time.sleep(0.8)
    out = b""
    os.write(fd, send)
    end = time.time() + settle
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


def main():
    # 1: the exact prompt from the report.
    out = render(UBUNTU_PS1)
    check("stock Ubuntu PS1: no literal \\033 on screen",
          "\\033" not in out, "colour escapes printed as text: %r" % out[:120])
    check("stock Ubuntu PS1: no literal ':+()}' on screen",
          ":+()}" not in out, "${v:+w} leaked its operator: %r" % out[:120])
    check("stock Ubuntu PS1: real ESC bytes reach the terminal",
          "\x1b[01;32m" in out, "no green span; got %r" % out[:120])
    check("stock Ubuntu PS1: \\u@\\h still renders",
          "@" in out, "got %r" % out[:120])

    # 2: \nnn octal in isolation. \101 is 'A', \033 is ESC.
    out = render(r"[\101\102\103]\$ ")
    check("octal escape \\101\\102\\103 renders as ABC", "[ABC]" in out,
          "got %r" % out[:120])

    # 3: the ${...} operator forms, against a variable we control.
    out = render(r"<${SET_VAR:+yes}|${UNSET_VAR:+no}|${UNSET_VAR:-dflt}>\$ ",
                 extra={"SET_VAR": "x"})
    check("${v:+w} expands when v is set", "<yes|" in out,
          "got %r" % out[:160])
    check("${v:+w} expands empty when v is unset", "|" in out and
          "no" not in out.split("<")[-1][:12], "got %r" % out[:160])
    check("${v:-default} falls back when v is unset", "dflt>" in out,
          "got %r" % out[:160])

    # 4: plain ${NAME} must still work (the reader this replaced did).
    out = render(r"<${SET_VAR}>\$ ", extra={"SET_VAR": "plain"})
    check("plain ${NAME} still expands", "<plain>" in out,
          "got %r" % out[:160])

    # 5: an unterminated ${ keeps the dollar literal, like bash.
    out = render(r"<${oops>\$ ")
    check("unterminated ${ keeps the '$' literal", "<${oops>" in out,
          "got %r" % out[:160])

    # 6: `set -o` must name the option it rejected (bash parity).
    out = render(r"\$ ", send=b"set -o notanoption\n")
    check("set -o names the rejected option",
          "notanoption: invalid option name" in out,
          "got %r" % out[-200:])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

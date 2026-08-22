#!/usr/bin/env python3
"""Regression test: the built-in prompt's live badges -- issue #50.

Reported as "the process tracker into the prompt disappeared". The
process tracker is the background-jobs badge, " ⚙N", that the built-in
two-row prompt shows on its info row while jobs exist; the duration badge
"took N.Ns" sits right next to it and broke in exactly the same way.

Root cause. prompt_normal() (src/infrastructure/prompt.c) refreshed three
process-mirror globals -- anim_jobs(), anim_dur_ms(), anim_status() -- and
only then rendered the built-in prompt. Those refreshes lived on the
PS1-is-unset branch, AFTER the early `return ps1_animated(...)` taken when
PS1 has a value:

    ps1 = env_expand(state, "PS1");
    if (ps1 && *ps1)
        return (ps1_animated(state, ps1));   <-- taken from 2.7.0 on
    ...
    *anim_jobs() = state->job_table.count;   <-- never reached any more
    render_prompt(...);

That branch was dead code until 5b6d3d4 shipped a default PS1 of "\B" (so
a virtualenv could restore it, issue #39). From that release on PS1 always
has a value, the early return is always taken, and render_extras() -- which
reads anim_jobs()/anim_dur_ms(), not the live job table -- rendered from
mirrors frozen at zero. The badges silently vanished for every user who had
not set a PS1 of their own.

The mirrors are now refreshed before the PS1 branch, so every route to the
built-in prompt sees the current process state.

Three routes are covered, because they reach render_extras() differently:
  1. PS1 unset      -- the historical built-in path
  2. PS1='\B'       -- the shipped default, the one users actually run
  3. PS1='...\J...' -- the \J escape, which reads the job table directly

Usage: python3 prompt_jobs_badge_test.py /path/to/hellish
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
                        else "build/bin/hellish")
FAILS = []

BASE_ENV = {
    "HOME": os.environ.get("HOME", "/tmp"),
    "PATH": os.environ["PATH"],
    "TERM": "xterm-256color", "LANG": "C.UTF-8",
    "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
    "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
}

GEAR = "⚙"          # ⚙ -- the process tracker glyph
ESC_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def session(cmds, ps1=None, cols=120, settle=1.4):
    """Drive an interactive hellish on a pty; return the plain-text screen.

    A wide pty on purpose: render_extras() DROPS the badges when the info
    row lacks room for them plus the clock, so a narrow terminal would hide
    the very thing under test and the regression would read as a pass.
    """
    env = dict(BASE_ENV)
    if ps1 is not None:
        env["PS1"] = ps1
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, cols, 0, 0))
    time.sleep(0.8)
    out = b""
    for cmd in cmds:
        os.write(fd, cmd)
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
    return ESC_RE.sub("", out.decode(errors="replace"))


def jobs_badge_case(label, ps1):
    """A background job must raise ⚙1; reaping it must lower the badge."""
    out = session([b"sleep 30 &\n", b"echo AFTER\n"], ps1=ps1)
    check("%s: background job raises the ⚙ tracker" % label,
          GEAR in out, "no gear on screen: %r" % out[-260:])
    check("%s: the tracker counts the job (⚙1)" % label,
          GEAR + "1" in out, "gear without its count: %r" % out[-260:])

    # And it must come back DOWN -- a badge that never clears is just as
    # wrong as one that never shows, and a stale mirror would keep it lit.
    out = session([b"sleep 30 &\n", b"kill %1\n", b"wait\n", b"echo REAPED\n"],
                  ps1=ps1)
    tail = out.split("REAPED", 1)[-1]
    check("%s: the tracker clears once no job is left" % label,
          GEAR not in tail, "gear outlived its job: %r" % tail[:260])


def main():
    # The shipped configuration: PS1 unset in the environment, so the shell
    # falls back to its own default. This is the exact case from the report.
    jobs_badge_case("default prompt", None)

    # The literal default value set_default_ps1() installs. Same renderer,
    # reached through the PS1 branch instead of the fallback.
    jobs_badge_case("PS1='\\B'", "\\B")

    # \J in a user PS1 reads state->job_table directly and never regressed;
    # it is here so a fix that "works" by breaking \J cannot pass.
    out = session([b"sleep 30 &\n", b"echo AFTER\n"], ps1=r"[\J]> ")
    check("\\J escape still renders the tracker", "[ " + GEAR + "1]" in out,
          "got %r" % out[-260:])

    # The duration badge shares the mirrors with the tracker and broke with
    # it, so it is pinned here too: 2s is render_extras()'s threshold.
    out = session([b"sleep 2.5\n", b"echo DONE\n"], ps1=None, settle=4.0)
    check("default prompt: 'took N.Ns' returns after a slow command",
          "took 2." in out, "no duration badge: %r" % out[-260:])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

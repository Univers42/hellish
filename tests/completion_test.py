#!/usr/bin/env python3
"""Regression test: tab completion, on the ft_malloc heap on purpose.

issue #40. readline OWNS every string a completion generator returns and
releases it with libc free(). Our generators built theirs with ft_strdup /
ft_substr / ft_strjoin, which compile to ft_malloc on a SAFE=0 build -- so
the first TAB handed libc a pointer libc never allocated:

    ✘130 ❯ free(): invalid size

(or "double free or corruption (out)", depending on what PATH happened to
hold) and the line editor never recovered. This is invisible on SAFE=1,
where xmalloc IS libc malloc and there is no mismatch to find, so this
test is only meaningful against a SAFE=0 binary -- `make completion-test`
builds one for exactly that reason.

Also covers the truncation found alongside it: cmd_gen_dirs closed each
PATH directory after its FIRST match, and readline asks for one match per
call, so every other command in that directory was silently dropped. An
empty TAB listed roughly one command per PATH entry instead of all of them.

A note on driving readline from a test, learned the hard way in CI: never
let a keystroke this test types survive to become a command. The empty-line
TAB case answers readline's "Display all N possibilities?" with `n`, and if
that question did not appear (one TAB only dings; it takes two), the `n`
lands on the command line instead. On a GitHub runner `n` is the node
version manager, which switches to the alternate screen and eats every
byte the test sends afterwards -- so `make completion-test` failed there
and passed on every developer machine, where `n` is not installed. Two
TABs to make the question deterministic, and a Ctrl-U before Enter so a
stray character is discarded either way.

Usage: python3 completion_test.py /path/to/hellish
"""
import fcntl
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

CRASH_MARKERS = ("free(): invalid size", "double free", "corrupted",
                 "munmap_chunk", "AddressSanitizer", "not malloc()-ed",
                 "SIGSEGV", "Aborted")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(sends, path_extra=None, settle=1.4):
    env = {
        "HOME": os.environ.get("HOME", "/tmp"),
        "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1",
        # the host's (or the user's) inputrc must not decide whether a TAB
        # dings, lists, or completes -- that is the thing under test.
        "INPUTRC": "/dev/null",
        # leaks are checked by verify_alloc; here we want heap ERRORS loud
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    if path_extra:
        env["PATH"] = path_extra + ":" + env["PATH"]
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 100, 0, 0))
    time.sleep(0.8)
    out = b""
    for data in sends:
        try:
            os.write(fd, data)
        except OSError:
            break
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


def no_crash(out):
    return [m for m in CRASH_MARKERS if m in out]


def launched_something(out):
    """True if a full-screen program took the terminal over.

    The alternate-screen switch is the fingerprint of a keystroke escaping
    to the command line and running a real host program (see the module
    docstring). It makes every later assertion in the case meaningless, so
    it is checked for explicitly instead of being diagnosed from a
    confusing tail= dump the next time CI goes red.
    """
    return "\x1b[?1049h" in out or "\x1b[?47h" in out


def main():
    # 1: the exact report -- TAB on an empty line. The second TAB is what
    # makes readline ask "Display all N possibilities?" (the first only
    # dings); 'n' declines it, and Ctrl-U throws the line away so that 'n'
    # can never be run as a command if the question did not appear.
    out = run([b"\t\t", b"n", b"\x15", b"\n", b"echo STILL_ALIVE\n"])
    hits = no_crash(out)
    check("TAB on an empty line does not corrupt the heap", not hits,
          "saw %s" % hits)
    check("readline offers the whole PATH, not a handful",
          "possibilities?" in out, "tail=%r" % out[-200:])
    check("the shell still runs commands after that TAB",
          out.count("STILL_ALIVE") >= 2, "tail=%r" % out[-200:])
    check("no keystroke this test typed launched a program",
          not launched_something(out), "tail=%r" % out[-200:])

    # 2: completing a builtin still works (the matches are libc-owned now,
    # so this is the path that would abort if rl_dup were reverted).
    out = run([b"expor\t", b"\n"])
    check("TAB completes a builtin name", "export" in out,
          "tail=%r" % out[-200:])
    check("completing a builtin does not corrupt the heap",
          not no_crash(out), "saw %s" % no_crash(out))

    # 3: variable completion -- ft_strjoin's intermediate was the other
    # cross-heap allocation.
    out = run([b"echo $HOM\t", b"\n"])
    check("TAB completes $HOME", "$HOME" in out or os.environ.get("HOME", "")
          in out, "tail=%r" % out[-200:])
    check("variable completion does not corrupt the heap",
          not no_crash(out), "saw %s" % no_crash(out))

    # 4: every match in a PATH directory is offered, not just the first.
    d = tempfile.mkdtemp(prefix="hellish_comp_")
    try:
        for name in ("zzprobe_alpha", "zzprobe_beta", "zzprobe_gamma"):
            p = os.path.join(d, name)
            with open(p, "w") as f:
                f.write("#!/bin/sh\n")
            os.chmod(p, 0o755)
        out = run([b"zzprobe_\t\t", b"\n"], path_extra=d, settle=1.8)
        found = [n for n in ("alpha", "beta", "gamma")
                 if "zzprobe_" + n in out]
        check("all matches in one PATH dir are offered, not just the first",
              len(found) == 3,
              "only %r survived the scan; tail=%r" % (found, out[-300:]))
    finally:
        shutil.rmtree(d, ignore_errors=True)

    # 5: the CI failure this file caused, made reproducible anywhere.
    # Shadow the decline key with a real, full-screen program on PATH --
    # which is exactly the runner's `n` -- and prove the drive sequence
    # never hands it to the shell. Without the Ctrl-U (or with a single
    # TAB, which only dings) this hangs and STILL_ALIVE never arrives.
    d = tempfile.mkdtemp(prefix="hellish_shadow_")
    try:
        p = os.path.join(d, "n")
        with open(p, "w") as f:
            f.write("#!/bin/sh\nprintf '\\033[?1049hTOOK-THE-SCREEN\\n'"
                    "\nsleep 30\n")
        os.chmod(p, 0o755)
        out = run([b"\t\t", b"n", b"\x15", b"\n", b"echo STILL_ALIVE\n"],
                  path_extra=d, settle=1.8)
        check("a one-letter program on PATH cannot hijack this test",
              not launched_something(out)
              and out.count("STILL_ALIVE") >= 2, "tail=%r" % out[-300:])
    finally:
        shutil.rmtree(d, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

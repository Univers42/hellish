#!/usr/bin/env python3
"""`complete -o ...`, and what counts as a command position.

Three defects, all of which made a real completion script behave worse
than no script at all:

  1. `-o` kept only its LAST value. git-completion registers
         complete -o bashdefault -o default -o nospace -F __git_wrap__git_main git
     so `default` -- the one that says "fall back to filenames when I find
     nothing" -- was thrown away by `-o nospace` arriving after it.

  2. The `-o` list was recorded and never read at all, so even a spec that
     did keep `default` could not act on it: progcomp claimed the word the
     moment a spec existed, and an empty answer from a completion function
     meant the user got nothing where bash gives them files.

  3. A word after a reserved word is a command name. `then`, `do`, `else`,
     `elif`, `in`, `!`, `time` -- and the wrappers `sudo`, `command`,
     `exec`, `env`, `nohup` -- all leave the next word in command
     position, and none of them were recognised, so
         for i in 1 2; do ec<TAB>
     offered the files in the current directory instead of `echo`.

A unique command name is planted on PATH for the command-position cases:
with an ambiguous prefix a single TAB only rings the bell, and the test
would be measuring how many binaries the machine happens to have.

Usage: python3 completion_opts_test.py /path/to/hellish
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
                        else "../build/bin/hellish")
UNIQ = "zzuniquecmd"
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def drain(fd, quiet=0.45, cap=8.0):
    """Read until the pty has been silent for `quiet` seconds. A fixed
    sleep per keystroke is a race that passes idle and fails under load;
    completion_posix_test.py learned that the hard way."""
    out = bytearray()
    last = time.monotonic()
    end = last + cap
    while time.monotonic() < end:
        r, _, _ = select.select([fd], [], [], 0.04)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out.extend(d)
            last = time.monotonic()
        elif time.monotonic() - last > quiet:
            break
    return bytes(out)


def clean(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]|[\x01\x02]", b"",
                  b).decode("utf-8", "replace")


def session(home, bindir, setup, typed):
    env = {
        "HOME": home, "PATH": bindir + ":" + os.environ.get("PATH", "/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        # The host's inputrc must not decide whether TAB rings or lists.
        "INPUTRC": "/dev/null",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "--norc"])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    drain(fd, 0.8, 5.0)
    setup_out = b""
    for line in setup:
        os.write(fd, (line + "\n").encode())
        setup_out += drain(fd, 0.3, 4.0)
    os.write(fd, typed)
    out = drain(fd, 0.6, 8.0)
    # Ctrl-U: whatever completion produced must never reach the shell as a
    # command when the session is torn down.
    os.write(fd, b"\x15")
    drain(fd, 0.2, 2.0)
    os.write(fd, b"exit\n")
    drain(fd, 0.3, 2.0)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return clean(setup_out), clean(out)


def main():
    home = tempfile.mkdtemp(prefix="hellish_compopts_")
    bindir = os.path.join(home, "bin")
    os.makedirs(bindir)
    with open(os.path.join(bindir, UNIQ), "w") as f:
        f.write("#!/bin/sh\necho ran\n")
    os.chmod(os.path.join(bindir, UNIQ), 0o755)
    with open(os.path.join(home, "target_file.txt"), "w") as f:
        f.write("x")
    pg = "shopt -s progcomp"

    # 3: command position after a reserved word / wrapper.
    for prefix in ("", "do ", "then ", "sudo ", "for i in 1 2; do ",
                   "if true; then "):
        _, out = session(home, bindir, [],
                         (prefix + UNIQ[:6]).encode() + b"\t")
        check("command position after %r" % (prefix or "<start of line>"),
              UNIQ in out, repr(out[-60:]))

    # 1: every -o value survives the parse.
    setup, _ = session(home, bindir,
                       [pg, "_f() { :; }",
                        "complete -o bashdefault -o default -o nospace"
                        " -F _f gitz", "complete -p gitz"], b"")
    line = re.search(r"complete .*gitz", setup)
    got = line.group(0) if line else ""
    for opt in ("bashdefault", "default", "nospace"):
        check("complete -p reports -o %s" % opt, opt in got,
              "printed %r" % got)

    # 2: the fallback the spec asked for, and the claim it did not.
    _, out = session(home, bindir,
                     [pg, "_none() { COMPREPLY=(); }",
                      "complete -o bashdefault -o default -F _none gitx"],
                     b"gitx target\t")
    check("-o default falls back to filenames on an empty spec",
          "target_file.txt" in out, repr(out[-60:]))
    _, out = session(home, bindir,
                     [pg, "_none() { COMPREPLY=(); }",
                      "complete -F _none gity"], b"gity target\t")
    check("without -o default an empty spec still claims the word",
          "target_file.txt" not in out, repr(out[-60:]))
    _, out = session(home, bindir,
                     [pg, "_w() { COMPREPLY=(wordy); }",
                      "complete -F _w gitw"], b"gitw wo\t")
    check("a spec that matches still inserts its match",
          "wordy" in out, repr(out[-60:]))

    shutil.rmtree(home, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""A completed word must survive being read back as shell input.

    touch 'my file.txt' ; cat my<TAB>
      bash: cat my\\ file.txt      here: cat my file.txt

The second one is not the file. It is two words, and the command runs
against neither, so the completion produced a line that cannot work --
worse than producing nothing. readline inserts what a generator returns
verbatim; bash installs four hooks to make the result quoted, and none of
them were set here.

Two more, from the same family:

  - `\\` was in the word-break set, and bash's has never had it. A
    backslash in a word is an ESCAPE, not a boundary, so continuing a name
    whose space was already escaped (`cat my\\ fi<TAB>`) cut the word at
    the backslash and could only ring the bell.

  - `complete -W` candidates were word-split by $IFS. compgen prints one
    candidate per LINE, but the array literal that collected them used the
    default IFS, so `complete -W 'a "b c" d'` offered b and c separately
    -- and with `IFS=:` set anywhere in the session, nothing split at all.

Every case is diffed against the pinned bash rather than a hand-written
expectation, because the question "what should a completed word look
like" is exactly the one a hand-written expectation gets wrong twice.

Usage: python3 completion_quoting_test.py /path/to/hellish
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
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def session(shell, typed, files=(), setup=()):
    home = tempfile.mkdtemp(prefix="hellish_cq_")
    for f in files:
        with open(os.path.join(home, f), "w") as fh:
            fh.write("x")
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        # The host's inputrc must not decide whether TAB rings or lists.
        "INPUTRC": "/dev/null",
    }
    if shell.endswith("hellish"):
        argv = [shell, "--norc"]
    else:
        argv = [shell, "--norc", "--noprofile"]
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(shell, argv)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
    buf = bytearray()

    def drain(quiet=0.5, cap=8.0):
        last = time.monotonic()
        end = last + cap
        start = len(buf)
        while time.monotonic() < end:
            r, _, _ = select.select([fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(fd, 65536)
                except OSError:
                    break
                if not d:
                    break
                buf.extend(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                break
        return bytes(buf[start:])

    drain(1.0, 6.0)
    for line in setup:
        os.write(fd, (line + "\n").encode())
        drain(0.35, 5.0)
    os.write(fd, typed)
    out = drain(0.7, 9.0)
    os.write(fd, b"\x15")
    drain(0.2, 2.0)
    os.write(fd, b"exit\n")
    drain(0.2, 2.0)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    shutil.rmtree(home, ignore_errors=True)
    text = re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]|[\x01\x02]", b"",
                  out).decode("utf-8", "replace").strip()
    return text.splitlines()[-1][-40:] if text else ""


SPACED = ["my file.txt"]
CASES = [
    ("a space in the name is escaped", b"cat my\t", SPACED, ()),
    ("an already-escaped word continues", b"cat my\\ fi\t", SPACED, ()),
    ("an open double quote is respected", b'cat "my fi\t', SPACED, ()),
    ("a plain name gains no quoting", b"cat plai\t", ["plain.txt"], ()),
    ("shell metacharacters are escaped", b"cat wei\t", ["weird(1).txt"], ()),
]


def main():
    if not os.path.exists(ORACLE):
        print("skip: no pinned oracle at %s -- run `make oracle`" % ORACLE)
        sys.exit(0)
    for name, typed, files, setup in CASES:
        got = session(SHELL, typed, files, setup)
        want = session(ORACLE, typed, files, setup)
        check(name, got == want, "hellish=%r bash=%r" % (got, want))

    # -W candidates are split on newlines, which is what compgen emits --
    # not on $IFS, which takes a multi-word candidate apart.
    spec = """complete -W 'alpha "b c" delta' foo"""
    got = session(SHELL, b"foo b\t", (), ("shopt -s progcomp", spec))
    check("a multi-word -W candidate stays one candidate",
          "b c" in got, repr(got))
    got = session(SHELL, b"foo al\t", (), ("shopt -s progcomp", "IFS=:", spec))
    check("IFS set in the session does not corrupt -W candidates",
          "alpha" in got, repr(got))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""An expansion error discards the line, and names the word -- bash parity.

Reported from a terminal:

    $ docker image ls --format ${{.ID}}
    hellish: ${{.ID}: bad substitution
    }
    }
    ...

Two things were wrong. The message named the ${...} span the scanner
stopped at -- `${{.ID}` -- where bash names the word, `${{.ID}}`. And the
command RAN: the reporter returned "" for the bad expansion and the rest of
the word, a lone `}`, went to docker as its --format, which printed a `}`
per image. bash's expansion errors jump to the top level with DISCARD:
the command does not run, nothing after its `;` runs, and $? is 1.

    $ echo ${{.ID}}; echo after
    bash: ${{.ID}}: bad substitution
    $ echo $?
    1

Non-interactive shells exit instead (127 under --posix), which the golden
suite pins; this is the interactive half, which only a terminal can show.

Usage: python3 bad_substitution_test.py [/path/to/hellish]
"""
import os
import pty
import select
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def session(cmds, settle=1.0):
    home = tempfile.mkdtemp(prefix="badsubst-")
    with open(os.path.join(home, ".hellishrc"), "w") as f:
        f.write("PS1='P$ '\n")
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "dumb", "LANG": "C.UTF-8", "HELLISH_NO_BANNER": "1",
           "HELLISH_NO_ANIM": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "ASAN_OPTIONS": "detect_leaks=0"}
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "-i"])
        os._exit(127)
    out = b""

    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d

    drain(2.0)
    for c in cmds:
        os.write(fd, c + b"\n")
        drain(settle)
    os.write(fd, b"exit\n")
    drain(0.5)
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return out.decode("utf-8", "replace")


def printed(out, s):
    """s appeared as an OUTPUT line -- not merely echoed back as part of
    the command that was typed."""
    return ("\r\n" + s + "\r\n") in out


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2

    out = session([b"echo ${{.ID}}; echo AFTER-1",
                   b"echo st=$?",
                   b"true; echo ${{.ID}} && echo AFTER-2",
                   b"unset u; echo ${u:?msg}; echo AFTER-3",
                   b"echo st=$?",
                   b"x=${{a}}; echo AFTER-4",
                   b"printf '%s\\n' STILL-HERE"])
    check("the word is named, braces and all",
          "hellish: ${{.ID}}: bad substitution" in out, repr(out[-600:]))
    check("the span alone is not what is named",
          "${{.ID}}: bad" in out and "${{.ID}: bad" not in out,
          repr(out[-600:]))
    check("the rest of the line is discarded after ;",
          not printed(out, "AFTER-1"), repr(out[-600:]))
    check("$? is 1 after the discarded line", printed(out, "st=1"),
          repr(out[-600:]))
    check("...and after &&", not printed(out, "AFTER-2"), repr(out[-600:]))
    check("${u:?} discards the line the same way",
          "hellish: u: msg" in out and not printed(out, "AFTER-3"),
          repr(out[-600:]))
    check("an assignment's bad value names the value",
          "hellish: ${{a}}: bad substitution" in out
          and not printed(out, "AFTER-4"), repr(out[-600:]))
    check("the shell is still there afterwards", printed(out, "STILL-HERE"),
          repr(out[-300:]))
    check("no stray `}` was ever run as an argument",
          "\r\n}\r\n" not in out, repr(out[-600:]))

    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

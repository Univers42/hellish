#!/usr/bin/env python3
"""A path that cannot run, typed at an interactive prompt (issue #133).

    ╰─❯ ./life
    hellish: ./life: No such file or directory
    <nothing: Enter only moved the cursor, until ^C>

The message was wrong -- the file was there -- and the prompt did not come
back. tests/scripts/53_exec_diagnostics.sh grades the words and statuses
against bash in a script; this runs the same failures where the report
happened, in a job-control shell on a terminal: the child takes the
terminal, fails, and the shell must take it back and prompt at once. Each
case must print bash's diagnostic, then a prompt, then answer `echo $?`
with bash's status. Waits on output: a hang is a timeout, a named failure.

The same failures through `exec` come after them. At a prompt a failed
exec must leave the shell up, as bash's does -- a script or subshell
exits instead (tests/scripts/55_exec_builtin.sh). `exec nosuch` used to
print "command not found", then read the shell it had just freed: under
ASan the shell died on the spot. exec names the file by its full path, as
bash's does (@D@ is the directory the cases run in).

Usage: python3 exec_error_tty_test.py [/path/to/hellish]
"""
import os
import shutil
import stat
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(HERE)
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

# what is typed, what the shell says about it, the status
CASES = [
    ("./notexec", b"./notexec: Permission denied", 126),
    ("./dir", b"./dir: Is a directory", 126),
    ("./nope", b"./nope: No such file or directory", 127),
    ("./nointerp",
     b"./nointerp: /nonexistent/interp: bad interpreter: No such file", 126),
    ("./elfjunk", b"./elfjunk: cannot execute binary file: Exec format error",
     126),
    ("./notexec/x", b"./notexec/x: Not a directory", 126),
    ("exec nosuch_cmd_q", b"exec: nosuch_cmd_q: not found", 127),
    ("exec -- nosuch_cmd_q", b"exec: nosuch_cmd_q: not found", 127),
    ("exec ./notexec", b"@D@/notexec: Permission denied", 126),
    ("exec ./dir", b"@D@/dir: Is a directory", 126),
    ("exec ./nope", b"@D@/nope: No such file or directory", 127),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def fixtures():
    d = tempfile.mkdtemp(prefix="exec-error-tty-")
    os.mkdir(os.path.join(d, "dir"))
    for name, data, mode in (
            ("notexec", b"echo not-run\n", 0o644),
            ("nointerp", b"#!/nonexistent/interp\necho ran\n", 0o755),
            ("elfjunk", b"\x7fELFnot-a-real-header\n", 0o755)):
        p = os.path.join(d, name)
        with open(p, "wb") as f:
            f.write(data)
        os.chmod(p, mode | stat.S_IRUSR)
    return d


def main():
    d = fixtures()
    t = Tty(SHELL)
    check("start/prompt", t.expect(b"P$ "), repr(t.out[-300:]))
    t.send("cd %s\r" % d)
    t.expect(b"P$ ")
    for cmd, said, status in CASES:
        said = said.replace(b"@D@", d.encode())
        mark = len(t.out)
        t.send(cmd + "\r")
        ok = t.expect(said, timeout=5) and t.expect(b"P$ ", timeout=5)
        check(cmd + "/says-why-and-prompts", ok, repr(t.since(mark)[-300:]))
        t.send("echo \"st=$?\"\r")
        ok = t.expect(b"st=%d\r\n" % status, timeout=5)
        check(cmd + "/status-%d" % status, ok and t.expect(b"P$ "),
              repr(t.since(mark)[-300:]))
    st = t.close()
    check("exit/not-signalled", os.WIFEXITED(st), "wait status %r" % st)
    shutil.rmtree(d, ignore_errors=True)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

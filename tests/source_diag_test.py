#!/usr/bin/env python3
"""A syntax error in a sourced file says where it is -- issue #113.

A 42 student's fresh install printed

    hellish: syntax error near unexpected token `('
    hellish: syntax error near unexpected token `('

at every start.  No file, no line, and twice: a dozen files load at
startup (rc.d, the plugin framework, every plugin, the imported ~/.zshrc)
and the report could not be acted on, let alone reproduced.  Two bugs in
the chunked exec_string path: the parse-all pass and the statement replay
BOTH printed, and the replay parsed everything up to the error as one
list, so `echo before` on line 1 never ran when line 2 was broken.

Bash, sourcing the same file, prints

    /path/bad.sh: line 2: syntax error near unexpected token `x'
    /path/bad.sh: line 2: `echo (x)'

after running line 1 -- and at an interactive prompt prefixes both lines
with "bash: ".  That is what this pins, for `source`, for rc.d/after.d
modules and for ~/.hellishrc, plus: an eval inside a sourced file reports
against ITS text, an unterminated group names the line bash names, the
issue-112 hint still points at its `}` line, and `return` at the top of
~/.hellishrc ends the file instead of erroring.

Usage: python3 source_diag_test.py [/path/to/hellish]
"""
import os
import pty
import re
import select
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FIXTURE = os.path.join(ROOT, "tests", "fixtures", "issue112.zshrc")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)


def run(argv, cwd, home=None):
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "HOME": home or cwd, "TERM": "dumb", "LANG": "C.UTF-8",
           "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_BANNER": "0",
           "ASAN_OPTIONS": "detect_leaks=0"}
    p = subprocess.run(argv, cwd=cwd, env=env, capture_output=True,
                       text=True, timeout=30)
    return p.returncode, p.stdout, p.stderr


def run_interactive(home, cmds, settle=1.0):
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "XDG_CONFIG_HOME": os.path.join(home, ".config"),
           "TERM": "xterm-256color", "LANG": "C.UTF-8",
           "HELLISH_BANNER": "0", "HELLISH_NO_UPDATE_CHECK": "1",
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
            r, _, _ = select.select([fd], [], [], 0.15)
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
        os.write(fd, c.encode() + b"\n")
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
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|[\x01\x02]",
                  "", out.decode("utf-8", "replace"))


def n_errors(text):
    return len(re.findall(r"syntax error", text))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    d = tempfile.mkdtemp(prefix="hellish-srcdiag-")
    try:
        # ---- source: one report, file and line, the line itself, the
        # healthy prefix has run, no "hellish:" prefix when not interactive
        bad = os.path.join(d, "bad.sh")
        write(bad, "echo before\necho (x)\necho after\n")
        rc, out, err = run([SHELL, "-c", "source " + bad], d)
        check("source/line 1 ran before the error on line 2",
              out == "before\n", "out=%r" % out)
        check("source/status 2", rc == 2, "rc=%d" % rc)
        check("source/exactly one syntax error", n_errors(err) == 1,
              "err=%r" % err)
        check("source/names the file and the line",
              ("%s: line 2: syntax error" % bad) in err, "err=%r" % err)
        check("source/echoes the offending line",
              ("%s: line 2: `echo (x)'" % bad) in err, "err=%r" % err)
        check("source/no shell-name prefix when not interactive",
              not err.startswith("hellish:"), "err=%r" % err)
        if shutil.which("bash"):
            _, bout, _ = run(["bash", "-c", "source " + bad], d)
            check("source/stdout matches bash", bout == out,
                  "bash=%r us=%r" % (bout, out))

        # ---- the error in a later chunk: the line is still the file's
        late = os.path.join(d, "late.sh")
        write(late, "alias ll=ls\necho l2\necho l3\necho (x)\n")
        rc, out, err = run([SHELL, "-c", "source " + late], d)
        check("chunks/lines before the alias hazard and after it ran",
              out == "l2\nl3\n", "out=%r" % out)
        check("chunks/line counted from the top of the file",
              ("%s: line 4:" % late) in err, "err=%r" % err)
        check("chunks/still one report", n_errors(err) == 1, "err=%r" % err)

        # ---- eval inside a sourced file reports against ITS text
        ev = os.path.join(d, "evalin.sh")
        write(ev, "echo l1\neval 'echo (x)'\necho l3\n")
        rc, out, err = run([SHELL, "-c", "source " + ev], d)
        check("eval/the file goes on after a failed eval",
              out == "l1\nl3\n", "out=%r" % out)
        check("eval/not blamed on the file's line",
              ("evalin.sh: line" not in err) and n_errors(err) == 1,
              "err=%r" % err)

        # ---- unterminated group: bash's line, once
        un = os.path.join(d, "unterm.sh")
        write(un, "echo a\nf() {\n  echo hi\n")
        rc, out, err = run([SHELL, "-c", "source " + un], d)
        check("eof/prefix ran", out == "a\n", "out=%r" % out)
        check("eof/line 4 like bash",
              ("%s: line 4: syntax error: unexpected end of file" % un)
              in err, "err=%r" % err)
        check("eof/once", n_errors(err) == 1, "err=%r" % err)

        # ---- the issue-112 hint keeps pointing at the `}` line
        if os.path.exists(FIXTURE):
            rc, out, err = run([SHELL, "-c", "source " + FIXTURE], d)
            check("hint/end of file at line 19 of the fixture",
                  "line 19: syntax error: unexpected end of file" in err,
                  "err=%r" % err[:300])
            check("hint/the `}' line is line 3",
                  re.search(r"line 3: a bare `}'", err) is not None,
                  "err=%r" % err[:300])

        # ---- interactive: an after.d module and ~/.hellishrc name
        # themselves, with the shell's name in front (bash: prefix)
        home = os.path.join(d, "home")
        mod = os.path.join(home, ".config", "hellish", "after.d", "50-x.zsh")
        write(mod, "echo mod-before\necho (x)\necho mod-after\n")
        rcf = os.path.join(home, ".hellishrc")
        write(rcf, "echo rc-before\nreturn\necho rc-after\n")
        out = run_interactive(home, ["echo done-$?"])
        check("interactive/after.d module: the line before ran",
              "mod-before" in out, "out=%r" % out[:400])
        check("interactive/after.d module: file and line, prefixed",
              ("hellish: %s: line 2: syntax error" % mod) in out,
              "out=%r" % out[:400])
        check("interactive/after.d module: once",
              n_errors(out) == 1, "out=%r" % out[:400])
        check("interactive/~/.hellishrc: return ends the file",
              "rc-before" in out and "rc-after" not in out
              and "can only `return'" not in out, "out=%r" % out[:400])
        check("interactive/the shell survives all of it",
              "done-" in out, "out=%r" % out[:400])
    finally:
        shutil.rmtree(d, ignore_errors=True)
    print("\n%d failure(s)" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

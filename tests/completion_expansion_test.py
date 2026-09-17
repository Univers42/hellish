#!/usr/bin/env python3
"""A completed path that the shell can still expand: `~`, `$VAR`.

Three field reports, one family. The word a user types to be EXPANDED was
being treated as a filename to be QUOTED, or not expanded at all:

    ls ~/.con<TAB>              ls \\~/.config/     (a directory named "~")
    mv build/bin/hellish ~/.c   mv ... \\~/.config/
    ls $HOME/.con<TAB>          nothing happens    (opendir("$HOME"))
    echo $HOM<TAB>              echo $HOME         (bash: $HOME/)

A backslashed `~` or `$` names something that does not exist, so the line
the user was handed could not run -- worse than completing nothing.

  quoting   complete_quote.c leaves the expandable HEAD of a word alone: a
            leading `~`, a leading `$NAME`/`${NAME}`. In the middle of a
            filename those characters are ordinary and still get escaped,
            which is what the bracket/dollar/space cases below pin.
  path      complete_dollar.c installs rl_directory_rewrite_hook, so
            opendir(2) reads through `$HOME/` while the line keeps the
            text the user typed, and rl_filename_stat_hook so a completed
            directory still earns its trailing '/'.
  append    a variable whose value is a directory appends '/' not ' '.

Every case is diffed against the pinned bash (tests/../bash-5.3.9, or
$BASH) rather than a hand-written expectation, for the reason
completion_quoting_test gives: what a completed word should look like is
exactly the question a hand-written expectation gets wrong twice. Without
that bash the file reports skip rather than inventing an answer.

Both readers run, in-process and HELLISH_RL_FORK=1 -- and the knob is
passed through explicitly, because an env dict built from scratch drops it
and then reports, very convincingly, that the two behave identically.

Usage: python3 completion_expansion_test.py /path/to/hellish
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
BASH = (os.environ.get("BASH_ORACLE")
        or os.path.expanduser("~/bash-5.3.9/bin/bash"))
FAILS = []
FILES = ["my file.txt", "bracket[1].txt", "dollar$var.txt", "plain.txt",
         "twin-one", "twin-two"]
DIRS = ["subdir"]

# (name, typed, keys). The home directory these run in gets FILES/DIRS, and
# $HOME itself is that directory, so ~ and $HOME reach a known tree.
CASES = [
    ("tilde dir",        "ls ~/sub", "\t"),
    ("tilde alone",      "cd ~/", "\t"),
    ("tilde bare",       "cd ~", "\t"),
    ("tilde after arg",  "mv x ~/sub", "\t"),
    ("tilde redirect",   "cat > ~/sub", "\t"),
    ("tilde pipe",       "true | ls ~/sub", "\t"),
    ("tilde assign",     "X=~/sub", "\t"),
    ("var path",         "ls $HOME/sub", "\t"),
    ("var path braced",  "ls ${HOME}/sub", "\t"),
    ("var path deeper",  "ls $HOME/subdir/", "\t"),
    ("var dir append",   "echo $HOM", "\t"),
    ("var unset stays",  "ls $NOPE_NOT_SET/x", "\t"),
    ("plain file",       "cat plai", "\t"),
    ("space escaped",    "cat my\\ fi", "\t"),
    ("space in quotes",  "cat 'my fi", "\t"),
    ("bracket literal",  "cat brack", "\t"),
    ("dollar literal",   "cat doll", "\t"),
    ("two matches",      "ls twin", "\t\t"),
    ("rel dir",          "ls ./sub", "\t"),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def make_home():
    home = tempfile.mkdtemp(prefix="hellish_cx_")
    for f in FILES:
        with open(os.path.join(home, f), "w") as fh:
            fh.write("x")
    for d in DIRS:
        os.makedirs(os.path.join(home, d), exist_ok=True)
    return home


def run_cases(shell, argv, extra):
    """One session, every case, ^U between. Returns {name: visible line}."""
    home = make_home()
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
        "PS1": "$ ", "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
        "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
        "HISTFILE": "/dev/null", "INPUTRC": "/dev/null",
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    for k in ("HELLISH_RL_FORK", "HELLISH_RL_INPROC"):
        if os.environ.get(k):
            env[k] = os.environ[k]
    env.update(extra)
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        fcntl.ioctl(0, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 120, 0, 0))
        os.execve(shell, [shell] + argv, env)
        os._exit(127)
    buf = bytearray()

    def pump(secs):
        end = time.monotonic() + secs
        while time.monotonic() < end:
            if select.select([fd], [], [], 0.03)[0]:
                try:
                    d = os.read(fd, 65536)
                except OSError:
                    return
                if not d:
                    return
                buf.extend(d)

    pump(1.5)
    out = {}
    for name, typed, keys in CASES:
        mark = len(buf)
        os.write(fd, typed.encode())
        pump(0.35)
        os.write(fd, keys.encode())
        pump(0.9)
        txt = bytes(buf[mark:]).decode("utf-8", "replace")
        txt = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b[78]|[\x07\x01\x02]", "", txt)
        rows = [r for r in txt.replace("\r", "\n").split("\n") if r.strip()]
        line = rows[-1] if rows else ""
        if line.startswith("$ "):
            line = line[2:]
        out[name] = line.rstrip()
        os.write(fd, b"\x15")
        pump(0.25)
    os.write(fd, b"exit\r")
    pump(0.4)
    try:
        os.kill(pid, 9)
    except OSError:
        pass
    os.waitpid(pid, 0)
    shutil.rmtree(home, ignore_errors=True)
    return out


def main():
    if not os.path.exists(BASH):
        print("skip: no pinned bash at %s (make oracle, or set "
              "BASH_ORACLE)" % BASH)
        sys.exit(0)
    want = run_cases(BASH, ["--norc", "--noprofile"], {})
    for reader, extra in (("in-process", {}),
                          ("forked", {"HELLISH_RL_FORK": "1"})):
        got = run_cases(SHELL, ["--norc"], extra)
        for name, typed, keys in CASES:
            check("%s: %s" % (reader, name), got[name] == want[name],
                  "typed %r -> hellish %r, bash %r"
                  % (typed + keys, got[name], want[name]))
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

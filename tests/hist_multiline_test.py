#!/usr/bin/env python3
"""Regression test: multi-line commands in interactive history.

Guards the bash-cmdhist joining of history_join.c. The old flattening
(space for every newline) made recalled entries syntactically broken
(`for i in 1 2 3 do ... done`), semantically different (quoted newline
-> space) or shell-wedging (a here-doc joined onto one line waits for a
terminator that never comes).

Every entry is joined ONCE, at the point it is recorded, so readline's
recall buffer, `history` / `fc -l` and the history file all show the same
text -- which is what bash does. Joining only for readline (issue #6) left
`history` printing the raw lines of a multi-line command: a for-loop came
out as three rows, only the first of them numbered.

Expectations below are measured against bash 5.3.9 in the same pty, not
assumed: a boundary newline becomes "; " (or a bare space where a ";"
would not parse, e.g. after `do`/`then` or a trailing `|`), a top-level
\\<newline> continuation disappears entirely, and newlines inside quotes
or a here-doc body stay literal.

Checks, in a real pty:
  1. `history` shows the bash-joined single-line form of a compound.
  2. Quoted and here-doc newlines survive the join.
  3. The history file round-trips the same joined text.
  4. Up-arrow recall of a here-doc re-executes correctly (no hang).
  5. A NEW session loads the file and recall of the for-loop still runs.
  6. while / pipe-continuation / case join the way bash joins them.

Usage: python3 hist_multiline_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import signal
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            os.execvp(SHELL, [SHELL])
            os._exit(127)
        self.drain(0.8)

    def drain(self, t=0.35):
        out = b""
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.08)
            if r:
                try:
                    c = os.read(self.fd, 65536)
                except OSError:
                    break
                if not c:
                    break
                out += c
        return out

    def send(self, s, wait=0.35):
        os.write(self.fd, s.encode())
        return self.drain(wait)

    def close(self):
        """exit; True when the shell terminated by itself (no wedge)."""
        self.send("exit\n", 0.5)
        for _ in range(30):
            try:
                p, _ = os.waitpid(self.pid, os.WNOHANG)
            except ChildProcessError:
                return True
            if p:
                return True
            time.sleep(0.1)
        try:
            os.kill(self.pid, signal.SIGKILL)
            os.waitpid(self.pid, 0)
        except (ProcessLookupError, ChildProcessError):
            pass
        return False


def plain(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b).decode(errors="replace")


def decode_hist_file(path):
    """Reverse of encode_cmd_hist: \\\\ -> \\, \\<nl> -> <nl>, bare <nl> ends."""
    entries, cur, bs = [], [], False
    with open(path, "rb") as f:
        data = f.read().decode(errors="replace")
    for c in data:
        if bs:
            cur.append(c)
            bs = False
        elif c == "\\":
            bs = True
        elif c == "\n":
            entries.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    return entries


# Kept in its own session so the up-arrow arithmetic in main() stays put.
# Each expectation is the exact string bash 5.3.9 records for that input.
SHAPES = [
    ("while", "while false\ndo echo W\ndone", "while false; do echo W; done"),
    ("pipe cont", "echo hi |\ncat", "echo hi | cat"),
    ("case", "case x in\nx) echo M ;;\nesac", "case x in x) echo M ;; esac"),
    ("brace group", "{ echo B\n}", "{ echo B; }"),
]


def more_shapes():
    home = tempfile.mkdtemp(prefix="hellish_shape_")
    s = Session(home)
    for _, cmd, _ in SHAPES:
        for line in cmd.split("\n"):
            s.send(line + "\n", 0.4)
    out = plain(s.send("history %d\n" % len(SHAPES), 0.9))
    for name, _, want in SHAPES:
        check("%s joins as bash does" % name, want in out,
              "want %r in %r" % (want, out[-400:]))
    check("shape session exits cleanly", s.close())


def main():
    home = tempfile.mkdtemp(prefix="hellish_hist_")
    cmds = [
        "for i in 1 2 3\ndo echo LOOP$i\ndone",
        'echo "A\nB"',
        "echo C\\\nD",
        "cat <<XEOF\nH1\nXEOF",
        "if true; then\n(echo SUB)\nfi",
    ]

    s = Session(home)
    for cmd in cmds:
        for line in cmd.split("\n"):
            s.send(line + "\n", 0.4)
    hist_out = plain(s.send("history 5\n", 0.9))
    check("history joins the loop the way bash does",
          "for i in 1 2 3; do echo LOOP$i; done" in hist_out,
          repr(hist_out[-500:]))
    check("history keeps the quoted newline literal",
          'echo "A' in hist_out and "B\"" in hist_out,
          repr(hist_out[-500:]))
    # recall the here-doc entry (2 ups: history-cmd, if-entry, -> 3 ups total
    # from history5; entries after 'history 5': ^=history5,1=if,2=heredoc)
    s.send("\x1b[A\x1b[A\x1b[A", 0.5)
    ran = plain(s.send("\n", 0.9))
    check("recalled heredoc re-executes", "H1" in ran, repr(ran[:300]))
    check("session exits cleanly (no heredoc wedge)", s.close())

    entries = decode_hist_file(os.path.join(home, ".minishell_history"))
    check("file keeps the joined loop",
          "for i in 1 2 3; do echo LOOP$i; done" in entries, repr(entries))
    check("file keeps quoted newline", cmds[1] in entries)
    check("file drops the \\<newline> continuation", "echo CD" in entries)
    check("file keeps heredoc", cmds[3] in entries)
    check("file joins if/subshell", "if true; then (echo SUB); fi" in entries)

    # fresh session: loaded-from-file entries must recall correctly too.
    # Over-shoot the up-arrows: readline pins at the OLDEST entry, which is
    # the for-loop, so the count stays right as earlier steps add entries.
    s2 = Session(home)
    s2.send("echo warm\n", 0.5)
    s2.send("\x1b[A" * 25, 1.0)
    out = plain(s2.send("\n", 1.0))
    check("reloaded loop recall runs", out.count("LOOP") >= 3, repr(out[:400]))
    check("second session exits cleanly", s2.close())
    more_shapes()

    print("== %s ==" % ("ALL PASSED" if not FAILS else
                        "%d FAILURES: %s" % (len(FAILS), ", ".join(FAILS))))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()

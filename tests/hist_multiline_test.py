#!/usr/bin/env python3
"""Regression test: multi-line commands in interactive history.

Guards the bash-cmdhist joining of history_join.c. The old flattening
(space for every newline) made recalled entries syntactically broken
(`for i in 1 2 3 do ... done`), semantically different (quoted newline
-> space) or shell-wedging (a here-doc joined onto one line waits for a
terminator that never comes).

Checks, in a real pty:
  1. `history` shows the original multi-line text (as it was typed).
  2. The history file round-trips the original (escaped \\<newline> form).
  3. Up-arrow recall of a here-doc re-executes correctly (no hang).
  4. A NEW session loads the file and recall of the for-loop still runs.

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
    check("history shows multiline loop", "for i in 1 2 3\r\ndo echo LOOP$i"
          in hist_out or "for i in 1 2 3\ndo echo LOOP$i" in hist_out,
          repr(hist_out[-500:]))
    # recall the here-doc entry (2 ups: history-cmd, if-entry, -> 3 ups total
    # from history5; entries after 'history 5': ^=history5,1=if,2=heredoc)
    s.send("\x1b[A\x1b[A\x1b[A", 0.5)
    ran = plain(s.send("\n", 0.9))
    check("recalled heredoc re-executes", "H1" in ran, repr(ran[:300]))
    check("session exits cleanly (no heredoc wedge)", s.close())

    entries = decode_hist_file(os.path.join(home, ".minishell_history"))
    check("file keeps loop multiline", cmds[0] in entries, repr(entries))
    check("file keeps quoted newline", cmds[1] in entries)
    check("file keeps continuation", cmds[2] in entries)
    check("file keeps heredoc", cmds[3] in entries)

    # fresh session: loaded-from-file entries must recall correctly too.
    # Over-shoot the up-arrows: readline pins at the OLDEST entry, which is
    # the for-loop, so the count stays right as earlier steps add entries.
    s2 = Session(home)
    s2.send("echo warm\n", 0.5)
    s2.send("\x1b[A" * 25, 1.0)
    out = plain(s2.send("\n", 1.0))
    check("reloaded loop recall runs", out.count("LOOP") >= 3, repr(out[:400]))
    check("second session exits cleanly", s2.close())

    print("== %s ==" % ("ALL PASSED" if not FAILS else
                        "%d FAILURES: %s" % (len(FAILS), ", ".join(FAILS))))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()

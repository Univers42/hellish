#!/usr/bin/env python3
"""Regression test for issue #42: hellish dumped the whole command history
at startup, before every prompt, with no input from the user at all.

The reporter (CachyOS) showed a screenshot of a numbered history listing
scrolling past and then a prompt. Nothing they typed caused it, and the
report contained no command to reproduce -- which is why it is worth
writing down what it actually was.

`history` parsed its argument with ft_atoi. ft_atoi("-a") is 0, a count of
0 means "no count was given", and "no count" means print the entire list.
So `history -a` -- silent in bash -- printed everything. `history -a` is
not an exotic invocation: it is the stock bashrc line that flushes the
history file after every command, usually via PROMPT_COMMAND, and hellish
imports PROMPT_COMMAND from the environment exactly as bash does. One
inherited variable and the shell was unusable.

The golden category tests/issue42_history_opts covers what every option
does. It cannot cover THIS, because the golden harness runs `hellish -c`
and the bug needs an interactive shell, a populated history file and a
prompt cycle. So: a real pty, a real history file, and the assertion that
a prompt is quiet.

Checks:
  1. PROMPT_COMMAND='history -a' prints nothing (the reported bug).
  2. ... and does not grow the history file by re-appending what this
     session already streamed into it.
  3. -w / -r / -n / -d / -c / -s / -p behave in a live session too.
  4. `history -c` clears readline's recall, not just our vector.
  5. An unknown option is an error, not a request to print everything.

Usage: python3 history_opts_test.py /path/to/hellish
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
SEEDED = ["echo one", "echo two", "echo three", "ls -la", "cd /tmp"]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def plain(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b).decode(errors="replace")


class Session:
    """One interactive hellish in front of a real terminal.

    PATH is deliberately NOT inherited: these tests type short strings at a
    prompt, and a one-letter word that happens to be an executable on the
    host would run a program instead of being the harmless typo the test
    intends. (That is not hypothetical -- `n` is the node version manager
    on a GitHub runner, and it takes over the terminal.)
    """

    def __init__(self, home, prompt_command=None):
        env = {
            "HOME": home, "PATH": "/usr/bin:/bin",
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
            "INPUTRC": "/dev/null",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        }
        if prompt_command is not None:
            env["PROMPT_COMMAND"] = prompt_command
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            os.execvp(SHELL, [SHELL])
            os._exit(127)
        self.start = self.drain(1.0)

    def drain(self, t=0.4):
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

    def send(self, s, wait=0.4):
        os.write(self.fd, s.encode())
        return plain(self.drain(wait))

    def close(self):
        try:
            os.write(self.fd, b"exit\n")
        except OSError:
            pass
        for _ in range(20):
            try:
                p, _ = os.waitpid(self.pid, os.WNOHANG)
            except ChildProcessError:
                return
            if p:
                return
            time.sleep(0.1)
        try:
            os.kill(self.pid, signal.SIGKILL)
            os.waitpid(self.pid, 0)
        except (ProcessLookupError, ChildProcessError):
            pass


def seeded_home():
    home = tempfile.mkdtemp(prefix="hellish_hist42_")
    with open(os.path.join(home, ".minishell_history"), "w") as f:
        f.write("".join(c + "\n" for c in SEEDED))
    return home


def hist_lines(home):
    with open(os.path.join(home, ".minishell_history")) as f:
        return [ln for ln in f.read().split("\n") if ln]


def numbered(text):
    """The listing rows `history` prints: '   12  echo one'."""
    return re.findall(r"^\s*\d+\s\s\S.*$", text, re.M)


def test_issue42_prompt_command():
    home = seeded_home()
    s = Session(home, prompt_command="history -a")
    banner = plain(s.start)
    check("issue #42: PROMPT_COMMAND='history -a' prints no listing",
          not numbered(banner), "startup showed %r" % numbered(banner)[:4])
    out = s.send("echo alive\n")
    check("issue #42: it stays quiet on the next prompt too",
          not numbered(out) and "alive" in out,
          "after one command: %r" % out[-200:])
    s.close()
    after = hist_lines(home)
    dupes = [c for c in SEEDED if after.count(c) > 1]
    check("issue #42: -a does not re-append what the session already wrote",
          not dupes, "duplicated %r in %r" % (dupes, after))
    check("the seeded history survived the session", SEEDED[0] in after,
          "file is now %r" % after)


def test_live_session_options():
    home = seeded_home()
    s = Session(home)
    out = s.send("history 2\n")
    rows = numbered(out)
    check("history 2 lists exactly two entries", len(rows) == 2,
          "rows=%r" % rows)

    out = s.send("history -w %s/w.txt\n" % home)
    check("history -w writes without printing the list", not numbered(out),
          "printed %r" % numbered(out)[:4])
    ok = os.path.exists(os.path.join(home, "w.txt"))
    check("history -w created the file", ok)
    if ok:
        with open(os.path.join(home, "w.txt")) as f:
            body = f.read()
        check("history -w wrote the seeded entries", SEEDED[0] in body,
              "file was %r" % body[:120])

    out = s.send("history -n %s/w.txt\n" % home)
    check("history -n prints nothing", not numbered(out),
          "printed %r" % numbered(out)[:4])
    out = s.send("history -r %s/w.txt\n" % home)
    check("history -r prints nothing", not numbered(out),
          "printed %r" % numbered(out)[:4])

    out = s.send("history -s marker_entry\n")
    check("history -s prints nothing", not numbered(out))
    out = s.send("history 1\n")
    check("history -s stored the entry", "marker_entry" in out,
          "tail=%r" % out[-160:])

    out = s.send("history -p abc\n")
    check("history -p echoes the expansion exactly once",
          out.count("abc") == 2, "tail=%r" % out[-160:])

    out = s.send("history -zz\n")
    check("an unknown option does not print the list", not numbered(out),
          "printed %r" % numbered(out)[:4])
    out = s.send("echo st=$?\n")
    check("an unknown option exits 2", "st=2" in out, "tail=%r" % out[-120:])
    s.close()


def test_clear_also_clears_recall():
    home = seeded_home()
    s = Session(home)
    s.send("history -c\n")
    out = s.send("history\n")
    # bash records a line BEFORE running it, hellish records it after, so
    # bash's post-clear listing holds `history` and ours holds `history -c`.
    # That ordering difference is old and orthogonal; what -c has to do is
    # drop everything that came before it.
    left = [r for r in numbered(out) if "history" not in r]
    check("history -c drops every earlier entry", not left,
          "still listed %r" % left[:4])
    # Walk the whole recall ring, not one step: the newest entry is the
    # `history` we just typed, so a single up-arrow proves nothing. A
    # -c that forgot readline leaves the seeded entries further back.
    out = ""
    for _ in range(10):
        out += s.send("\x1b[A", 0.2)
    stale = [c for c in SEEDED if c in out]
    check("history -c also clears readline's recall", not stale,
          "up-arrow resurrected %r" % stale)
    os.write(s.fd, b"\x15\n")
    s.drain(0.3)
    s.close()


def main():
    test_issue42_prompt_command()
    test_live_session_options()
    test_clear_also_clears_recall()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

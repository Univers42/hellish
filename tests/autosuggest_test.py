#!/usr/bin/env python3
"""Inline autosuggestion (#140): the newest history entry that starts with
what is typed, dim after the cursor, zsh-autosuggestions style.

OFF unless HELLISH_AUTOSUGGEST is 1 / on. Right, End and ^F accept it; with
nothing to accept they are the plain keys. It is never part of the line, so
what is checked is what a terminal would show: a pyte screen for the cells
right of the cursor and the cursor column, the raw bytes for the dim
attribute (pyte has no SGR 2), and the output of the lines that are run.

Cells: both readers (in-process and HELLISH_RL_FORK=1). Each case gets a
fresh shell and a history file of its own.

Partial accept (Alt-F, Ctrl-Right): the ghost's leading blanks plus the
next run of non-blank bytes. Cycling (Alt-N older, Alt-P newer): distinct
matching entries, no wrap (bell at either end), reset by any edit. With no
history ghost these keys are their stock readline functions.

Ranking (HELLISH_SUGGEST_STRATEGY): by default the entry with the best
frecency (how often and how recently its text was run; weight 2^16 halved
per 64 newer entries, summed per distinct text, ties to the newest) is the
ghost and the order Alt-N / Alt-P walk; `recent` is newest first. `model()`
below is the same rule written again in Python, and the cases check the
shell against it as well as against hand-computed answers.

Usage: python3 autosuggest_test.py /path/to/hellish [case-name-part [inproc]]
"""
import fcntl
import os
import pty
import select
import signal
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []
ROWS = 30
PROMPT = "IN#> "
PW = len(PROMPT)
DIM = b"\x1b[2m"
ESC = "\x1b"
RIGHT, RIGHT_O = "\x1b[C", "\x1bOC"
END, END_O = "\x1b[F", "\x1bOF"
LEFT = "\x1b[D"
ALT_F, ALT_N, ALT_P = "\x1bf", "\x1bn", "\x1bp"
CTRL_RIGHT = "\x1b[1;5C"
CTRL_A, CTRL_E = "\x01", "\x05"
CTRL_F, CTRL_U, CTRL_C, CTRL_D, CTRL_R, CTRL_G = (
    "\x06", "\x15", "\x03", "\x04", "\x12", "\x07")
BKSP = "\x7f"
ON = "HELLISH_AUTOSUGGEST=1"
STRAT = "HELLISH_SUGGEST_STRATEGY"

try:
    import pyte

    class Screen(pyte.Screen):
        """pyte dispatches CSI with parameters; save/restore take none."""

        def save_cursor(self, *_):
            super().save_cursor()

        def restore_cursor(self, *_):
            super().restore_cursor()

    HAVE_PYTE = True
except ImportError:  # pragma: no cover
    HAVE_PYTE = False


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, home, env=None, rc=None, cols=100, fork=False):
        e = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "HISTFILE": os.path.join(home, "hist"), "PS1": PROMPT,
            "ASAN_OPTIONS": "detect_leaks=" + os.environ.get("AS_LEAKS", "0"),
        }
        if fork:
            e["HELLISH_RL_FORK"] = "1"
        e.update(env or {})
        args = [SHELL, "--norc"]
        if rc:
            path = os.path.join(home, "rc")
            with open(path, "w") as f:
                f.write(rc)
            args = [SHELL, "--rcfile=" + path]
        self.cols = cols
        self.buf = bytearray()
        self.screen = Screen(cols, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            fcntl.ioctl(0, termios.TIOCSWINSZ,
                        struct.pack("HHHH", ROWS, cols, 0, 0))
            os.chdir(home)
            os.execve(SHELL, args, e)
            os._exit(127)
        self.until(lambda: b"#> " in self.buf, 10.0)
        self.settle()

    def pump(self, secs):
        r, _, _ = select.select([self.fd], [], [], secs)
        if not r:
            return False
        try:
            d = os.read(self.fd, 65536)
        except OSError:
            return False
        if not d:
            return False
        self.buf.extend(d)
        self.stream.feed(d)
        return True

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def settle(self, quiet=0.25, cap=3.0):
        end = time.monotonic() + cap
        while time.monotonic() < end and self.pump(quiet):
            pass

    def run(self, line):
        n = self.buf.count(b"#> ")
        os.write(self.fd, line.encode() + b"\r")
        self.until(lambda: self.buf.count(b"#> ") > n, 8.0)
        self.settle()

    def keys(self, data):
        if isinstance(data, str):
            data = data.encode()
        self.mark = len(self.buf)
        os.write(self.fd, data)
        self.settle()

    def since(self):
        """The bytes the terminal got since the last keys()."""
        return bytes(self.buf[self.mark:])

    def row(self, y=None):
        y = self.screen.cursor.y if y is None else y
        line = self.screen.buffer[y]
        return "".join(line[x].data for x in range(self.cols)).rstrip()

    def x(self):
        return self.screen.cursor.x

    def ghost(self):
        """What is on the cursor's row right of the cursor."""
        y = self.screen.cursor.y
        line = self.screen.buffer[y]
        return "".join(line[x].data
                       for x in range(self.x(), self.cols)).rstrip()

    def below_blank(self):
        y = self.screen.cursor.y
        return all(self.row(r) == "" for r in range(y + 1, ROWS))

    def screen_text(self):
        return "\n".join(self.row(y) for y in range(ROWS))

    def alive(self):
        try:
            pid, _ = os.waitpid(self.pid, os.WNOHANG)
        except ChildProcessError:
            return False
        return pid == 0

    def close(self):
        try:
            os.write(self.fd, b"\x05\x15exit\r")
            self.until(lambda: not self.alive(), 3.0)
            self.settle(0.2, 1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        check("sanitizer: no AddressSanitizer / LeakSanitizer report",
              b"Sanitizer" not in self.buf, repr(bytes(self.buf[-300:])))
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        os.close(self.fd)


def shell(fork, seed=(), on=True, **kw):
    """A shell with autosuggest on (unless `on` is False) and `seed` run."""
    home = tempfile.mkdtemp(prefix="autosuggest_")
    s = Session(home, fork=fork, **kw)
    if on:
        s.run(ON)
    for line in seed:
        s.run(line)
    s.run("clear")
    return s


def case_off_by_default(fork, tag):
    s = shell(fork, ["echo hello"], on=False)
    s.keys("ech")
    check(tag + "1 off by default: nothing right of the cursor",
          s.ghost() == "" and DIM not in s.since(), repr(s.ghost()))
    s.close()


def case_on(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("ech")
    check(tag + "2 on: ghost right of the cursor",
          s.ghost() == "o hello", repr(s.ghost()))
    check(tag + "2 on: the cursor stays after the typed text",
          s.x() == PW + 3, str(s.x()))
    check(tag + "2 on: painted dim", DIM + b"o hello" in s.since(),
          repr(s.since()))
    check(tag + "2 on: ghost is not part of the line",
          s.row() == PROMPT + "echo hello", s.row())
    s.keys(CTRL_U)
    check(tag + "2 on: ^U takes it away", s.ghost() == "", s.ghost())
    s.close()


def case_accept(fork, tag):
    for name, key in (("Right", RIGHT), ("SS3 Right", RIGHT_O),
                      ("End", END), ("SS3 End", END_O),
                      ("^F", CTRL_F)):
        s = shell(fork, ["echo hello"])
        s.keys("ech")
        s.keys(key)
        check(tag + "3 %s accepts: line and cursor" % name,
              s.row() == PROMPT + "echo hello" and s.x() == PW + 10
              and s.ghost() == "", "%r x=%d" % (s.row(), s.x()))
        start = len(s.buf)
        s.run("")
        out = bytes(s.buf[start:]).decode("utf-8", "replace")
        check(tag + "3 %s: Enter runs the accepted line" % name,
              "hello\n" in out.replace("\r", ""), repr(out))
        s.close()


def case_enter_plain(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("ech")
    s.keys("\r")
    s.settle()
    rows = [s.row(y) for y in range(ROWS)]
    mine = [r for r in rows if r.startswith(PROMPT + "ech")]
    check(tag + "4 Enter without accepting runs only what was typed",
          "hello" not in s.screen_text() and mine
          and all(r == PROMPT + "ech" for r in mine), repr(rows[:6]))
    s.close()


def case_newest(fork, tag):
    s = shell(fork, ["echo one", "echo two"])
    s.keys("echo ")
    check(tag + "5 most recent wins", s.ghost() == "two", repr(s.ghost()))
    s.close()


def case_edit(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("xyz")
    check(tag + "6 no match, no ghost", s.ghost() == "", repr(s.ghost()))
    s.keys(CTRL_U + "ech")
    s.keys("q")
    check(tag + "6 diverging clears it", s.ghost() == "", repr(s.ghost()))
    s.keys(BKSP)
    check(tag + "6 Backspace to a match brings it back",
          s.ghost() == "o hello", repr(s.ghost()))
    s.keys(BKSP * 2)
    check(tag + "6 shorter prefix, longer ghost",
          s.ghost() == "cho hello", repr(s.ghost()))
    s.keys(BKSP)
    check(tag + "6 empty line, no ghost", s.ghost() == "", repr(s.ghost()))
    s.close()


def case_cursor(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("ech")
    s.keys(LEFT)
    check(tag + "7 cursor not at the end: no ghost",
          s.ghost() == "h" and s.row() == PROMPT + "ech"
          and DIM not in s.since(), repr((s.ghost(), s.row())))
    s.keys(RIGHT)
    check(tag + "7 Right mid-line only moves the cursor",
          s.x() == PW + 3 and s.row() == PROMPT + "echo hello",
          "x=%d %r" % (s.x(), s.row()))
    s.keys(RIGHT)
    check(tag + "7 Right at the end accepts",
          s.row() == PROMPT + "echo hello", s.row())
    s.keys(CTRL_U + "zzz" + LEFT * 2)
    s.keys(END)
    check(tag + "7 End with no ghost is end-of-line",
          s.x() == PW + 3 and s.row() == PROMPT + "zzz", str(s.x()))
    s.keys(LEFT)
    s.keys(CTRL_F)
    check(tag + "7 ^F with no ghost is forward-char",
          s.x() == PW + 3 and s.row() == PROMPT + "zzz", str(s.x()))
    s.close()


def case_interrupt(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("ech")
    s.keys(CTRL_C)
    check(tag + "8 ^C leaves no ghost on the old row",
          all("o hello" not in s.row(y) for y in range(ROWS)),
          repr(s.screen_text()[:120]))
    check(tag + "8 ^C: a clean prompt", s.row() == PROMPT.rstrip(),
          repr(s.row()))
    s.keys("ech")
    check(tag + "8 ^C: the next prompt still suggests",
          s.ghost() == "o hello", repr(s.ghost()))
    s.keys(CTRL_U)
    s.keys(CTRL_D)
    s.until(lambda: not s.alive(), 3.0)
    check(tag + "8 ^D on an empty line exits", not s.alive())
    s.close()


def case_history_keys(fork, tag):
    s = shell(fork, ["echo one", "echo two", "ls /nonexistent_zz"])
    s.keys("echo")
    s.keys(b"\x1b[A")
    check(tag + "9 Up still searches by prefix",
          s.row() == PROMPT + "echo two", s.row())
    s.keys(b"\x1b[A")
    check(tag + "9 Up again", s.row() == PROMPT + "echo one", s.row())
    s.keys(b"\x1b[B")
    check(tag + "9 Down", s.row() == PROMPT + "echo two", s.row())
    s.close()


def case_search(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys(CTRL_R)
    s.keys("ech")
    check(tag + "10 no ghost while searching",
          DIM not in s.since() and "reverse-i-search" in s.row(),
          repr(s.row()))
    s.keys(CTRL_G)
    s.keys("ech")
    check(tag + "10 and again once it is over",
          s.ghost() == "o hello", repr(s.ghost()))
    s.close()


def case_vi(fork, tag):
    s = shell(fork, ["echo hello"])
    s.run("set -o vi")
    s.keys("ech")
    check(tag + "11 vi insert mode shows the ghost",
          s.ghost() == "o hello", repr(s.ghost()))
    s.keys(ESC)
    time.sleep(0.8)
    s.settle()
    check(tag + "11 vi command mode shows none",
          s.row() == "(cmd)" + PROMPT + "ech", repr(s.row()))
    s.keys("ao")
    check(tag + "11 back in insert mode it is back",
          s.ghost() == " hello" and s.x() == 5 + PW + 4, repr(s.ghost()))
    s.close()


def case_escape(fork, tag):
    s = Session(tempfile.mkdtemp(prefix="autosuggest_"), fork=fork)
    s.run(ON)
    s.run("echo ok")
    s.keys(b"echo \x16\x1b[31mRED\x16\x1b[0m # zzz\r")
    s.settle()
    s.run("clear")
    s.keys("echo")
    row = s.screen.buffer[s.screen.cursor.y]
    check(tag + "12 an entry holding ESC is never the ghost",
          s.ghost() == " ok", repr(s.ghost()))
    check(tag + "12 no ESC from the entry reaches the terminal",
          b"\x1b[31m" not in s.since() and b"zzz" not in s.since(),
          repr(s.since()))
    check(tag + "12 no colour on the row",
          all(row[x].fg == "default" for x in range(s.cols)))
    s.keys(CTRL_U)
    s.run("clear")
    s.run("printf '\\e[31mRED\\e[0m\\n' # yyy")
    s.run("clear")
    s.keys("printf")
    check(tag + "12 a literal backslash-e stays text, no colour",
          b"\x1b[31m" not in s.since()
          and all(s.screen.buffer[s.screen.cursor.y][x].fg == "default"
                  for x in range(s.cols)), repr(s.since()))
    s.close()


def case_multiline(fork, tag):
    s = shell(fork, [])
    s.keys("for i in 1 2\r")
    s.keys("do echo L$i\r")
    s.keys("done\r")
    s.settle()
    s.run("history -s $'while false\\ndo :\\ndone'")
    s.run("clear")
    s.keys("while")
    check(tag + "13 an entry with a newline is never suggested",
          s.ghost() == "" and DIM not in s.since(), repr(s.ghost()))
    s.keys(CTRL_U + "for")
    check(tag + "13 a typed loop never paints a newline",
          "\n" not in s.ghost() and s.below_blank(), repr(s.ghost()))
    s.close()


def case_truncate(fork, tag):
    s = shell(fork, ["echo " + "x" * 60], cols=40)
    s.keys("echo")
    check(tag + "14 long ghost is cut to the row",
          s.ghost() == " " + "x" * 29 and s.x() == PW + 4
          and s.below_blank(), "x=%d %r" % (s.x(), s.ghost()))
    s.keys(CTRL_U)
    s.close()


def case_utf8(fork, tag):
    s = shell(fork, ["echo h\u00e9llo w\u00f6rld"])
    s.keys("echo h\u00e9")
    check(tag + "15 UTF-8: ghost right after the typed text",
          s.ghost() == "llo w\u00f6rld" and s.x() == PW + 7,
          "x=%d %r" % (s.x(), s.ghost()))
    s.keys(RIGHT)
    check(tag + "15 UTF-8: accepted whole",
          s.row() == PROMPT + "echo h\u00e9llo w\u00f6rld", s.row())
    s.close()


def case_rprompt(fork, tag):
    s = shell(fork, ["echo hello", "echo " + "y" * 95],
              env={"RPROMPT": "RP"})
    s.keys("echo h")
    check(tag + "16 ghost and RPROMPT both shown",
          s.row().startswith(PROMPT + "echo hello")
          and s.row().endswith("RP") and s.x() == PW + 6, repr(s.row()))
    s.keys(CTRL_U + "echo")
    check(tag + "16 ghost reaching RPROMPT hides it",
          not s.row().endswith("RP") and "yyy" in s.row()
          and s.below_blank(), repr(s.row()))
    s.keys(CTRL_U)
    check(tag + "16 RPROMPT comes back with the ghost gone",
          s.row().endswith("RP") and s.row().startswith(PROMPT),
          repr(s.row()))
    s.close()


def case_paste(fork, tag):
    s = shell(fork, ["echo " + "p" * 100])
    t0 = time.monotonic()
    s.keys(b"echo " + b"p" * 4000)
    s.settle()
    check(tag + "17 4 KB paste: no hang", s.alive()
          and time.monotonic() - t0 < 10.0)
    s.keys(CTRL_U)
    s.run("echo done")
    check(tag + "17 4 KB paste: shell usable, no garbage rows",
          "ppp" not in s.screen_text() and "done" in s.screen_text(),
          repr(s.screen_text()[:200]))
    s.close()


def case_max(fork, tag):
    s = Session(tempfile.mkdtemp(prefix="autosuggest_"), fork=fork)
    s.run(ON)
    s.run("HELLISH_SUGGEST_MAX=1")
    s.run("echo old")
    s.run("ls /nonexistent_zz")
    s.run("clear")
    s.keys("echo")
    check(tag + "18 SUGGEST_MAX=1 looks at the newest entry only",
          s.ghost() == "", repr(s.ghost()))
    s.keys(CTRL_U)
    s.run("HELLISH_SUGGEST_MAX=abc")
    s.keys("echo")
    check(tag + "18 non-numeric is the default",
          s.ghost() == " old", repr(s.ghost()))
    s.keys(CTRL_U)
    s.run("HELLISH_SUGGEST_MAX=0")
    s.keys("echo")
    check(tag + "18 zero is the default",
          s.ghost() == " old", repr(s.ghost()))
    s.close()


def case_unset(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("ech")
    check(tag + "19 on", s.ghost() == "o hello", repr(s.ghost()))
    s.keys(CTRL_U)
    s.run("unset HELLISH_AUTOSUGGEST")
    s.keys("ech")
    check(tag + "19 unset: next line has no ghost",
          s.ghost() == "" and DIM not in s.since(), repr(s.ghost()))
    s.keys(RIGHT)
    check(tag + "19 unset: Right is forward-char",
          s.row() == PROMPT + "ech" and s.x() == PW + 3, s.row())
    s.keys(LEFT + RIGHT)
    check(tag + "19 unset: Left, Right moves the cursor only",
          s.x() == PW + 3 and s.row() == PROMPT + "ech", s.row())
    s.keys(CTRL_U)
    s.run("HELLISH_AUTOSUGGEST=ON")
    s.keys("ech")
    check(tag + "19 ON (any case) turns it back on",
          s.ghost() == "o hello", repr(s.ghost()))
    s.keys(CTRL_U)
    s.run("HELLISH_AUTOSUGGEST=0")
    s.keys("ech")
    check(tag + "19 0 is off", s.ghost() == "", repr(s.ghost()))
    s.close()


def case_user_bind(fork, tag):
    home = tempfile.mkdtemp(prefix="autosuggest_")
    s = Session(home, fork=fork, rc="HELLISH_AUTOSUGGEST=1\n"
                "bind '\"\\e[C\": backward-char'\n")
    s.run("echo hello")
    s.run("clear")
    s.keys("ech")
    check(tag + "20 the ghost is still shown", s.ghost() == "o hello",
          repr(s.ghost()))
    s.keys(RIGHT)
    check(tag + "20 the user's binding wins on Right",
          s.x() == PW + 2 and s.row() != PROMPT + "echo hello", s.row())
    s.keys("\x01\x0b" + "ech")
    s.keys(END)
    check(tag + "20 the other accept keys are still ours",
          s.row() == PROMPT + "echo hello", s.row())
    s.close()


POST_RC = ("setopt zsh\n"
           "HELLISH_AUTOSUGGEST=1\n"
           "w() { POSTDISPLAY=' <hint>'; }\n"
           "zle -N w\nbindkey '^X^H' w\n"
           "r() { BUFFER=\"$BUFFER$POSTDISPLAY\"; }\n"
           "zle -N r\nbindkey '^X^R' r\n")


def case_postdisplay(fork, tag):
    home = tempfile.mkdtemp(prefix="autosuggest_")
    s = Session(home, fork=fork, rc=POST_RC)
    s.run("echo hello")
    s.run("clear")
    s.keys("abc")
    s.keys(b"\x18\x08")
    check(tag + "21 a widget's POSTDISPLAY is painted",
          s.ghost() == "<hint>" or s.ghost() == " <hint>"
          and s.x() == PW + 3, "x=%d %r" % (s.x(), s.ghost()))
    check(tag + "21 painted dim", DIM + b" <hint>" in s.since(),
          repr(s.since()))
    s.keys("d")
    check(tag + "21 gone when the buffer changes", s.ghost() == "",
          repr(s.ghost()))
    s.keys(CTRL_U + "ech")
    s.keys(b"\x18\x12")
    check(tag + "21 a widget reads the suggestion from POSTDISPLAY",
          s.row() == PROMPT + "echo hello", s.row())
    s.keys("\x01\x0b")
    s.keys(b"\x18\x12")
    check(tag + "21 POSTDISPLAY is empty when there is none",
          s.row() == PROMPT.rstrip(), repr(s.row()))
    s.close()


def case_resize(fork, tag):
    s = shell(fork, ["echo " + "z" * 50])
    s.keys("echo")
    s.screen.resize(ROWS, 30)
    s.cols = 30
    fcntl.ioctl(s.fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, 30, 0, 0))
    s.settle(0.4, 4.0)
    g = s.ghost()
    check(tag + "22 resize with a ghost: alive, never wrapped",
          s.alive() and len(g) <= 30 - 1 - (PW + 4)
          and set(g) <= set("z "), repr(g))
    s.keys(CTRL_U)
    s.run("echo alive")
    check(tag + "22 resize: shell still usable", "alive" in s.screen_text(),
          repr(s.screen_text()[:100]))
    s.close()


def case_dumb(fork, tag):
    s = shell(fork, ["echo hello"], env={"TERM": "dumb"})
    s.keys("ech")
    check(tag + "23 TERM=dumb: no ghost",
          DIM not in s.since() and s.ghost() == "", repr(s.since()))
    s.close()



def case_word(fork, tag):
    s = shell(fork, ["git commit -m msg"])
    s.keys("git")
    check(tag + "24 W1 ghost before", s.ghost() == " commit -m msg",
          repr(s.ghost()))
    s.keys(ALT_F)
    check(tag + "24 W1 Alt-F takes ' commit'",
          s.row() == PROMPT + "git commit -m msg" and s.x() == PW + 10
          and s.ghost() == " -m msg", "x=%d %r" % (s.x(), s.ghost()))
    s.keys(ALT_F)
    check(tag + "24 W1 Alt-F again takes ' -m'",
          s.x() == PW + 13 and s.ghost() == " msg",
          "x=%d %r" % (s.x(), s.ghost()))
    s.keys(ALT_F)
    check(tag + "24 W1 last word, no ghost left",
          s.x() == PW + 17 and s.ghost() == ""
          and s.row() == PROMPT + "git commit -m msg",
          "x=%d %r" % (s.x(), s.ghost()))
    start = len(s.buf)
    s.run("")
    out = bytes(s.buf[start:]).decode("utf-8", "replace")
    check(tag + "24 W1 the line that runs is the accepted one",
          "git commit -m msg" in out or "command not found" in out
          or "not a git" in out or "git:" in out, repr(out))
    s.close()


def case_word_ctrl_right(fork, tag):
    s = shell(fork, ["git commit -m msg"])
    s.keys("git")
    s.keys(CTRL_RIGHT)
    s.keys(CTRL_RIGHT)
    check(tag + "25 W2 Ctrl-Right = Alt-F",
          s.x() == PW + 13 and s.ghost() == " msg",
          "x=%d %r" % (s.x(), s.ghost()))
    s.keys(CTRL_U + "git")
    s.keys("\x1b[5C")
    check(tag + "25 W2 CSI 5C form too", s.x() == PW + 10
          and s.ghost() == " -m msg", "x=%d %r" % (s.x(), s.ghost()))
    s.close()


def case_word_blanks(fork, tag):
    s = shell(fork, ["echo  two  spaces"])
    s.keys("echo")
    s.keys(ALT_F)
    check(tag + "26 W3 leading blanks come with the word",
          s.x() == PW + 9 and s.ghost() == "  spaces"
          and s.row() == PROMPT + "echo  two  spaces",
          "x=%d %r" % (s.x(), s.ghost()))
    s.close()


def case_word_stock(fork, tag):
    s = shell(fork, ["echo history"])
    s.keys("foo bar baz")
    check(tag + "27 W4 no ghost to start with", s.ghost() == "")
    s.keys(CTRL_A + ALT_F)
    check(tag + "27 W4 Alt-F is forward-word, inserts nothing",
          s.x() == PW + 3 and s.row() == PROMPT + "foo bar baz",
          "x=%d %r" % (s.x(), s.row()))
    s.keys(CTRL_E + CTRL_U + "echo")
    s.keys(LEFT)
    s.keys(ALT_F)
    check(tag + "27 W5 cursor not at the end: stock forward-word (the "
          "ghost, back at the end, is still only a ghost)",
          s.x() == PW + 4 and s.ghost() == " history"
          and s.row() == PROMPT + "echo history",
          "x=%d %r" % (s.x(), s.row()))
    s.keys(CTRL_A + RIGHT)
    s.keys(ALT_F)
    check(tag + "27 W5 mid-line: moves over the word, inserts nothing",
          s.x() == PW + 4 and s.row() == PROMPT + "echo history",
          "x=%d %r" % (s.x(), s.row()))
    s.close()


def case_word_utf8(fork, tag):
    s = shell(fork, ["echo h\u00e9llo w\u00f6rld"])
    s.keys("echo")
    s.keys(ALT_F)
    check(tag + "28 W6 UTF-8 word, cursor in columns",
          s.row() == PROMPT + "echo h\u00e9llo w\u00f6rld"
          and s.x() == PW + 10 and s.ghost() == " w\u00f6rld",
          "x=%d %r %r" % (s.x(), s.ghost(), s.row()))
    s.keys(ALT_F)
    check(tag + "28 W6 second UTF-8 word", s.x() == PW + 16
          and s.ghost() == "", "x=%d %r" % (s.x(), s.ghost()))
    s.close()


def three(fork, extra=(), **kw):
    return shell(fork, ["echo a1", "echo a2", "echo a3"] + list(extra), **kw)


def case_cycle(fork, tag):
    s = three(fork)
    s.keys("echo a")
    check(tag + "29 C1 newest first", s.ghost() == "3", repr(s.ghost()))
    for key, name, want in ((ALT_N, "Alt-N", "2"), (ALT_N, "Alt-N", "1"),
                            (ALT_N, "Alt-N at the oldest: bell, no wrap", "1"),
                            (ALT_P, "Alt-P", "2"), (ALT_P, "Alt-P", "3"),
                            (ALT_P, "Alt-P at the newest: bell", "3")):
        s.keys(key)
        check(tag + "29 C1 " + name, s.ghost() == want
              and s.row() == PROMPT + "echo a" + want
              and s.x() == PW + 6, "%r %r" % (s.ghost(), s.row()))
    s.keys(ALT_N + ALT_N + ALT_N)
    check(tag + "29 C1 bell rung at the oldest end", b"\a" in s.since(),
          repr(s.since()))
    s.close()


def case_cycle_dupes(fork, tag):
    """The pre-frecency rule, pinned: HELLISH_SUGGEST_STRATEGY=recent."""
    s = shell(fork, ["echo a1", "echo a2", "echo a1", "echo a3"],
              env={STRAT: "recent"})
    s.keys("echo a")
    seen = [s.ghost()]
    for _ in range(4):
        s.keys(ALT_N)
        seen.append(s.ghost())
    check(tag + "30 C2 recent: distinct entries, once each, then the end",
          seen == ["3", "1", "2", "2", "2"], repr(seen))
    s.close()


def case_cycle_dupes_frecent(fork, tag):
    """Same history, default strategy: a1 was run twice, so it leads."""
    s = shell(fork, ["echo a1", "echo a2", "echo a1", "echo a3"])
    s.keys("echo a")
    seen = [s.ghost()]
    for _ in range(4):
        s.keys(ALT_N)
        seen.append(s.ghost())
    check(tag + "30 C2 frecent: twice-run a1 first, once each, the end",
          seen == ["1", "3", "2", "2", "2"], repr(seen))
    s.close()


def case_cycle_reset(fork, tag):
    s = three(fork, ["echo ab9"])
    s.keys("echo a")
    s.keys(ALT_N + ALT_N)
    check(tag + "31 C3 two steps back", s.ghost() == "2", repr(s.ghost()))
    s.keys("b")
    check(tag + "31 C3 a typed character: newest match for the new line",
          s.ghost() == "9", repr(s.ghost()))
    s.keys(BKSP)
    check(tag + "31 C3 Backspace: newest again", s.ghost() == "b9",
          repr(s.ghost()))
    s.keys(ALT_N)
    check(tag + "31 C3 a new cycle starts from the newest",
          s.ghost() == "3", repr(s.ghost()))
    s.close()


def case_cycle_accept(fork, tag):
    s = three(fork)
    s.keys("echo a")
    s.keys(ALT_N)
    s.keys(RIGHT)
    check(tag + "32 C4 Right takes the alternative on show",
          s.row() == PROMPT + "echo a2" and s.ghost() == "", s.row())
    s.keys(CTRL_U + "echo a")
    s.keys(ALT_N + ALT_N)
    s.keys(ALT_F)
    check(tag + "32 C4 so does Alt-F, and the rest follows that entry",
          s.row() == PROMPT + "echo a1", s.row())
    s.keys(CTRL_U + "echo a")
    s.keys(ALT_N)
    s.keys("\r")
    s.settle()
    rows = [s.row(y) for y in range(ROWS)]
    check(tag + "32 C4 Enter runs only what was typed: the output is 'a'",
          PROMPT + "echo a" in rows and "a" in rows
          and "a2" not in rows and "a3" not in rows, repr(rows[:8]))
    s.close()


def case_cycle_max(fork, tag):
    s = Session(tempfile.mkdtemp(prefix="autosuggest_"), fork=fork,
                env={"HELLISH_AUTOSUGGEST": "1", "HELLISH_SUGGEST_MAX": "2"})
    for n in (1, 2, 3):
        s.run("echo a%d" % n)
    s.keys("echo a")
    seen = [s.ghost()]
    s.keys(ALT_N)
    seen.append(s.ghost())
    s.keys(ALT_N)
    seen.append(s.ghost())
    check(tag + "33 C5 SUGGEST_MAX=2 bounds the cycle",
          seen == ["3", "2", "2"], repr(seen))
    s.close()


def case_cycle_stock(fork, tag):
    s = shell(fork, ["echo hello"])
    s.keys("zzz")
    s.keys(ALT_P)
    check(tag + "34 C6 no ghost: Alt-P is the non-incremental search "
          "(readline's ':' prompt)", s.row() == PROMPT + ":", repr(s.row()))
    s.keys(CTRL_G)
    s.keys(CTRL_U + "zzz")
    s.keys(ALT_N)
    check(tag + "34 C6 no ghost: Alt-N is the non-incremental search",
          s.row() == PROMPT + ":", repr(s.row()))
    s.keys(CTRL_G)
    check(tag + "34 C6 still alive", s.alive())
    s.close()


def case_cycle_post(fork, tag):
    home = tempfile.mkdtemp(prefix="autosuggest_")
    s = Session(home, fork=fork, rc=POST_RC)
    s.run("echo abc1")
    s.run("echo abc2")
    s.run("clear")
    s.keys("abc")
    s.keys(b"\x18\x08")
    before = s.ghost()
    s.keys(ALT_N)
    check(tag + "35 C7 POSTDISPLAY is not cycled: Alt-N is the stock "
          "non-incremental search (its own prompt, no history line shown)",
          before.strip() == "<hint>" and "abc" not in s.row()
          and s.row().startswith("IN#>"),
          "%r %r" % (before, s.row()))
    s.keys(CTRL_G)
    check(tag + "35 C7 search aborted: the line is back",
          s.row().startswith(PROMPT + "abc") and s.alive(), repr(s.row()))
    s.keys(CTRL_U + "abc")
    s.keys(b"\x18\x08")
    s.keys(ALT_F)
    check(tag + "35 C7 nor taken a word at a time: stock forward-word, "
          "the hint is still the widget's", s.ghost() == before
          and s.x() == PW + 3 and s.row() == PROMPT + "abc" + before,
          "%r x=%d" % (s.ghost(), s.x()))
    s.close()


def case_cycle_user_bind(fork, tag):
    home = tempfile.mkdtemp(prefix="autosuggest_")
    s = Session(home, fork=fork, rc="HELLISH_AUTOSUGGEST=1\n"
                "bind '\"\\en\": backward-char'\n"
                "bind '\"\\ef\": backward-char'\n")
    s.run("echo a1")
    s.run("echo a2")
    s.run("clear")
    s.keys("echo a")
    s.keys(ALT_N)
    check(tag + "36 C8 the user's Alt-N wins (backward-char)",
          s.x() == PW + 5 and s.row() == PROMPT + "echo a",
          "x=%d %r" % (s.x(), s.row()))
    s.keys(CTRL_E + ALT_F)
    check(tag + "36 C8 the user's Alt-F wins",
          s.x() == PW + 5 and s.row() == PROMPT + "echo a", str(s.x()))
    s.keys(CTRL_E + ALT_P)
    check(tag + "36 C8 Alt-P is still ours (a bell, not a search prompt)",
          s.row() == PROMPT + "echo a2" and s.ghost() == "2", s.row())
    s.close()


def bound_keys(s, func):
    """The `bind -q` answer for a readline function, as the shell prints it."""
    s.run("clear")
    s.run("bind -q " + func)
    return s.screen_text()


def case_cycle_off(fork, tag):
    nis = "non-incremental-forward-search-history"
    s = shell(fork, ["echo a1", "echo a2"], on=False)
    check(tag + "37 C9 off: Alt-N, Alt-P, Alt-F, Ctrl-Right stay on their "
          "stock functions (bind -q)",
          '"\\en"' in bound_keys(s, nis)
          and '"\\ep"' in bound_keys(
              s, "non-incremental-reverse-search-history")
          and '"\\ef"' in bound_keys(s, "forward-word")
          and '"\\e[1;5C"' in bound_keys(s, "forward-word"),
          repr(bound_keys(s, nis)))
    s.keys("echo a")
    s.keys(ALT_N)
    check(tag + "37 C9 off: Alt-N is stock (search prompt)",
          s.ghost() == "" and s.row() == PROMPT + ":", repr(s.row()))
    s.keys(CTRL_G)
    s.keys(CTRL_U + "echo a")
    s.keys(ALT_P)
    check(tag + "37 C9 off: Alt-P is stock", s.ghost() == ""
          and s.row() == PROMPT + ":", repr(s.row()))
    s.keys(CTRL_G)
    s.keys(CTRL_U + "foo bar")
    s.keys(CTRL_A + ALT_F)
    check(tag + "37 C9 off: Alt-F is forward-word", s.x() == PW + 3
          and s.row() == PROMPT + "foo bar", str(s.x()))
    s.keys(CTRL_E + CTRL_U)
    s.run("HELLISH_AUTOSUGGEST=1")
    check(tag + "37 C9 switched on at the prompt: the keys are ours",
          "not bound" in bound_keys(s, nis), repr(bound_keys(s, nis)))
    s.keys("echo a")
    s.keys(ALT_N)
    check(tag + "37 C9 switched on at the prompt: Alt-N cycles",
          s.ghost() == "1", repr(s.ghost()))
    s.keys(CTRL_U)
    s.run("unset HELLISH_AUTOSUGGEST")
    check(tag + "37 C9 switched off again: stock keys are back",
          '"\\en"' in bound_keys(s, nis), repr(bound_keys(s, nis)))
    s.keys("echo a")
    s.keys(ALT_N)
    check(tag + "37 C9 switched off again: Alt-N is stock",
          s.ghost() == "" and s.row() == PROMPT + ":", repr(s.row()))
    s.keys(CTRL_G)
    s.close()


def case_cycle_vi(fork, tag):
    s = three(fork)
    s.run("set -o vi")
    s.keys("echo a")
    check(tag + "38 C10 vi insert: ghost", s.ghost() == "3", repr(s.ghost()))
    s.keys(ALT_N)
    check(tag + "38 C10 vi insert: Alt-N cycles", s.ghost() == "2",
          repr(s.ghost()))
    s.keys(ALT_P)
    check(tag + "38 C10 vi insert: Alt-P cycles back", s.ghost() == "3",
          repr(s.ghost()))
    s.keys(CTRL_U + "echo")
    s.keys(ALT_F)
    check(tag + "38 C10 vi insert: Alt-F takes a word",
          s.ghost() == "" and s.row().endswith("echo a3"), repr(s.row()))
    s.keys(CTRL_U + "echo a")
    s.keys(ESC)
    time.sleep(0.8)
    s.settle()
    check(tag + "38 C10 vi command mode: no ghost",
          s.row() == "(cmd)" + PROMPT + "echo a", repr(s.row()))
    s.close()

def case_cycle_history_shrinks(fork, tag):
    """A bind -x widget that clears the history mid-cycle must not leave the
    cycle pointing at entries that are gone (an out-of-bounds read)."""
    s = three(fork, ["bind -x '\"\\C-t\": history -c'"])
    s.keys("echo a")
    s.keys(ALT_N + ALT_N)
    check(tag + "29 C11 cycled before the clear", s.ghost() == "1",
          repr(s.ghost()))
    s.keys("\x14")
    s.keys(ALT_P)
    check(tag + "29 C11 no ghost survives a cleared history",
          s.ghost() == "" and s.alive(), repr(s.ghost()))
    s.keys("\x07\x15")
    s.run("echo ok")
    check(tag + "29 C11 the line still runs", "ok" in s.screen_text(),
          repr(s.screen_text()[-300:]))
    s.close()



def clean_text(t):
    """The ghost's filter: no control byte, no DEL, no C1 (as UTF-8)."""
    return not any(ord(c) < 0x20 or ord(c) == 0x7f or 0x80 <= ord(c) < 0xa0
                   for c in t)


def model(lines, prefix, strategy="frecent", mx=1000):
    """The ranking, written again: distinct texts, best first."""
    n = len(lines)
    score, newest = {}, {}
    for i in range(max(0, n - mx), n):
        t = lines[i]
        if not (t.startswith(prefix) and len(t) > len(prefix)
                and clean_text(t)):
            continue
        if strategy == "recent":
            score[t] = i + 1
        else:
            score[t] = score.get(t, 0) + (65536 >> min((n - 1 - i) // 64, 16))
        newest[t] = i
    return sorted(score, key=lambda t: (-score[t], -newest[t]))


def seeded(fork, lines, env=None, **kw):
    """A shell whose history is exactly `lines` (oldest first): nothing is
    run, so no other entry is in it, and autosuggest is on from the start."""
    home = tempfile.mkdtemp(prefix="autosuggest_")
    with open(os.path.join(home, "hist"), "w") as f:
        f.write("".join(line + "\n" for line in lines))
    e = {"HELLISH_AUTOSUGGEST": "1"}
    e.update(env or {})
    return Session(home, fork=fork, env=e, **kw)


def walk(s, typed, steps):
    """The ghost after `typed`, then after each of `steps` Alt-N."""
    s.keys(typed)
    seen = [s.ghost()]
    for _ in range(steps):
        s.keys(ALT_N)
        seen.append(s.ghost())
    return seen


# re is run three times, clean twice and newest, test once: frecency says
# re, the newest entry says clean.
ISSUE = ["make re", "make test", "make re", "make clean", "make re",
         "make clean"]


def case_frecent_issue(fork, tag):
    s = seeded(fork, ISSUE)
    got = walk(s, "make ", 3)
    check(tag + "37 F1 frecent: the most-run command, not the newest",
          got == ["re", "clean", "test", "test"], repr(got))
    s.close()
    s = seeded(fork, ISSUE, env={STRAT: "recent"})
    got = walk(s, "make ", 3)
    check(tag + "37 F1 recent: the newest, repeats collapsed",
          got == ["clean", "re", "test", "test"], repr(got))
    s.close()


def case_frecent_recent(fork, tag):
    lines = ["echo a1", "echo b1", "echo a2", "echo b2", "echo a3"]
    s = seeded(fork, lines, env={STRAT: "recent"})
    got = walk(s, "echo ", 4)
    check(tag + "38 F2 recent is newest first, as before frecency",
          got == ["a3", "b2", "a2", "b1", "a1"], repr(got))
    s.close()


def case_frecent_default(fork, tag):
    for val in ("", "bogus", "RECENT", "recent "):
        s = seeded(fork, ISSUE, env={STRAT: val})
        got = walk(s, "make ", 0)
        check(tag + "39 F3 %r is frecent" % val, got == ["re"], repr(got))
        s.close()
    s = seeded(fork, ISSUE)
    got = walk(s, "make ", 0)
    check(tag + "39 F3 unset is frecent", got == ["re"], repr(got))
    s.close()


def case_frecent_live(fork, tag):
    s = seeded(fork, ISSUE)
    steps = (("", "re"), (STRAT + "=recent", "clean"),
             (STRAT + "=bogus", "re"), (STRAT + "=recent", "clean"),
             ("unset " + STRAT, "re"))
    for cmd, want in steps:
        if cmd:
            s.run(cmd)
        got = walk(s, "make ", 0)
        check(tag + "40 F4 after %r the ghost is %r" % (cmd, want),
              got == [want], repr(got))
        s.keys(CTRL_U)
    s.close()


def case_frecent_tie(fork, tag):
    lines = ["echo x2", "echo x1", "echo x2", "echo x1"]
    s = seeded(fork, lines)
    got = walk(s, "echo x", 2)
    check(tag + "41 F5 equal scores: the newest first",
          got == ["1", "2", "2"], repr(got))
    s.close()
    s = seeded(fork, lines[1:] + ["echo x2"])
    got = walk(s, "echo x", 2)
    check(tag + "41 F5 equal scores, other way round",
          got == ["2", "1", "1"], repr(got))
    s.close()


def decay_lines(total):
    """fa twice at the very start, filler, fb newest: `total` entries."""
    lines = ["echo fa", "true 0", "echo fa"]
    lines += ["true %d" % n for n in range(1, total - 3)]
    return lines + ["echo fb"]


def case_frecent_decay(fork, tag):
    # Two fa at ages total-1 and total-3. Weight 2^16 halves each 64 newer
    # entries: at 66 entries (ages 65, 63) 2^15 + 2^16 beats fb's 2^16; at
    # 67 (ages 66, 64) both have halved, 2^16 ties, and the newest wins.
    for total, want in ((20, "fa"), (66, "fa"), (67, "fb"), (300, "fb")):
        lines = decay_lines(total)
        s = seeded(fork, lines)
        got = walk(s, "echo f", 0)
        check(tag + "42 F6 %d entries: %s wins" % (total, want),
              got == [want[1]] and model(lines, "echo f")[0] == "echo " + want,
              repr(got))
        s.close()


def case_frecent_window(fork, tag):
    lines = ["echo wa", "echo wb", "echo wa", "echo wb", "echo wa",
             "echo wc", "echo wb"]
    s = seeded(fork, lines)
    got = walk(s, "echo w", 0)
    check(tag + "43 F7 whole history: wa 3, wb 3, wc 1: the tie goes to "
          "the newest", got == ["b"], repr(got))
    s.close()
    s = seeded(fork, lines, env={"HELLISH_SUGGEST_MAX": "3"})
    got = walk(s, "echo w", 3)
    check(tag + "43 F7 MAX=3: wa, wc, wb once each, newest first",
          got == ["b", "c", "a", "a"], repr(got))
    s.close()
    lines = ["echo wa", "echo wa", "echo wb", "echo wa", "echo wb",
             "echo wc"]
    s = seeded(fork, lines)
    got = walk(s, "echo w", 0)
    check(tag + "43 F7 MAX unset: wa x3 wins", got == ["a"], repr(got))
    s.close()
    s = seeded(fork, lines, env={"HELLISH_SUGGEST_MAX": "4"})
    got = walk(s, "echo w", 2)
    check(tag + "43 F7 MAX=4: the two older wa do not count, wb x2 wins",
          got == ["b", "c", "a"], repr(got))
    s.close()


def case_frecent_cycle(fork, tag):
    s = seeded(fork, ISSUE)
    s.keys("make ")
    order = []
    for _ in range(3):
        order.append(s.ghost())
        s.keys(ALT_N)
    check(tag + "44 F8 Alt-N follows the score", order == ["re", "clean",
          "test"] and s.ghost() == "test", repr(order))
    s.keys(ALT_N)
    check(tag + "44 F8 bell at the last, ghost stays",
          b"\a" in s.since() and s.ghost() == "test", repr(s.since()))
    s.keys(ALT_P + ALT_P)
    check(tag + "44 F8 Alt-P walks back to the best",
          s.ghost() == "re" and s.row() == PROMPT + "make re",
          repr(s.row()))
    s.keys(ALT_P)
    check(tag + "44 F8 bell at the best, no wrap",
          b"\a" in s.since() and s.ghost() == "re", repr(s.since()))
    s.close()


def case_frecent_dirty(fork, tag):
    lines = ["echo ok", "echo \x1b[31mred", "echo \x1b[31mred",
             "echo \x1b[31mred", "echo ok2"]
    s = seeded(fork, lines)
    got = walk(s, "echo ", 3)
    check(tag + "45 F9 a frequent entry holding ESC is never offered",
          got == ["ok2", "ok", "ok", "ok"] and b"\x1b[31m" not in s.buf,
          repr(got))
    s.close()


def case_frecent_history_shrinks(fork, tag):
    s = seeded(fork, ISSUE)
    s.run("bind -x '\"\\C-t\": history -c'")
    s.keys("make ")
    s.keys(ALT_N)
    check(tag + "46 F10 cycled before the clear", s.ghost() == "clean",
          repr(s.ghost()))
    s.keys("\x14")
    s.keys(ALT_N)
    check(tag + "46 F10 no ghost survives a cleared history (Alt-N)",
          s.ghost() == "" and s.alive(), repr(s.ghost()))
    s.keys(CTRL_U + "make ")
    s.keys(ALT_N + ALT_P)
    check(tag + "46 F10 nor a cycle begun afterwards",
          s.ghost() == "" and s.alive(), repr(s.ghost()))
    s.keys("\x07\x15")
    s.run("echo ok")
    check(tag + "46 F10 the shell still runs lines",
          "ok" in s.screen_text(), repr(s.screen_text()[-200:]))
    s.close()


def case_frecent_typing(fork, tag):
    long_a = "echo abcdefghijklmnopqrstuvwxyz0123456789"
    long_b = "echo abcdefgXYZ0123456789abcdefghij"
    lines = [long_a, long_b, long_a, "echo abc", long_a]
    s = seeded(fork, lines)
    bad = []
    for path in (long_a, long_b):
        s.keys(CTRL_U + path[:4])
        for k in range(5, len(path) + 1):
            s.keys(path[k - 1])
            want = model(lines, path[:k])
            want = want[0][k:] if want else ""
            if s.ghost() != want:
                bad.append((path[:k], s.ghost(), want))
    check(tag + "47 F11 typing forward: every ghost is the best match for "
          "what is typed (%d keys)" % (len(long_a) + len(long_b) - 10),
          not bad, repr(bad[:3]))
    s.close()


def random_history(rng):
    words = ["alpha", "alps", "alt", "amber", "ant"]
    n = rng.choice([8, 40, 70, 130, 200])
    lines = []
    for i in range(n):
        w = rng.choice(words)
        if rng.random() < 0.5:
            lines.append("echo %s" % w)
        else:
            lines.append("echo %s %d" % (w, rng.randrange(3)))
        if i and lines[-1] == lines[-2]:
            lines[-1] += " x"
    return lines


def case_frecent_model(fork, tag):
    import random
    rng = random.Random(140 + int(fork))
    for k in range(5):
        lines = random_history(rng)
        mx = rng.choice([1000, 1000, 50])
        prefix = rng.choice(["echo ", "echo al", "echo a"])
        s = seeded(fork, lines, env={"HELLISH_SUGGEST_MAX": str(mx)})
        want = model(lines, prefix, mx=mx)
        got = walk(s, prefix, len(want) + 1)
        exp = [t[len(prefix):] for t in want]
        exp = (exp + exp[-1:] * 2)[:len(want) + 2] if exp else [""]
        exp = exp[:len(got)] if exp != [""] else [""] * len(got)
        check(tag + "48 F12 random history %d (%d entries, max %d, %r): "
              "the cycle equals the model" % (k, len(lines), mx, prefix),
              got == exp, "got %r\nwant %r" % (got, exp))
        s.close()



def case_frecent_sorted_worst(fork, tag):
    """20000 copies of one entry arrive already in ranking order, the worst
    case of a last-element-pivot quicksort (O(n^2)): the first key must
    still paint its ghost quickly."""
    n = 20000
    s = seeded(fork, ["ls -la"] * n + ["pwd"],
               env={"HELLISH_SUGGEST_MAX": str(n + 1),
                    "HISTSIZE": str(n + 10), "HISTFILESIZE": str(n + 10)})
    s.mark = len(s.buf)
    t0 = time.time()
    os.write(s.fd, b"l")
    s.until(lambda: s.ghost() == "s -la", 20.0)
    dt = time.time() - t0
    check(tag + "37 F13 sorted worst case paints within 1.5 s",
          s.ghost() == "s -la" and dt < 1.5, "%r %.2fs" % (s.ghost(), dt))
    s.close()



CASES = (case_off_by_default, case_on, case_accept, case_enter_plain,
         case_newest, case_edit, case_cursor, case_interrupt,
         case_history_keys, case_search, case_vi, case_escape,
         case_multiline, case_truncate, case_utf8, case_rprompt,
         case_paste, case_max, case_unset, case_user_bind,
         case_postdisplay, case_resize, case_dumb, case_word,
         case_word_ctrl_right, case_word_blanks, case_word_stock,
         case_word_utf8, case_cycle, case_cycle_dupes,
         case_cycle_dupes_frecent, case_cycle_reset,
         case_cycle_accept, case_cycle_max, case_cycle_stock,
         case_cycle_post, case_cycle_user_bind, case_cycle_off,
         case_cycle_vi, case_cycle_history_shrinks, case_frecent_issue,
         case_frecent_recent, case_frecent_default, case_frecent_live,
         case_frecent_tie, case_frecent_decay, case_frecent_window,
         case_frecent_cycle, case_frecent_dirty,
         case_frecent_history_shrinks, case_frecent_typing,
         case_frecent_model, case_frecent_sorted_worst)


def main():
    if not HAVE_PYTE:
        print("skip: needs pyte (python3 -m pip install pyte)")
        sys.exit(0)
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    only = sys.argv[2] if len(sys.argv) > 2 else ""
    readers = (False,) if sys.argv[3:4] == ["inproc"] else (False, True)
    for fork in readers:
        tag = "forked/" if fork else "in-process/"
        for case in CASES:
            if only in case.__name__:
                case(fork, tag)
    if FAILS:
        print("\n%d FAILED" % len(FAILS))
        sys.exit(1)
    print("\nall clear")


main()

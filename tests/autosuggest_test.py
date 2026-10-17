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
CTRL_F, CTRL_U, CTRL_C, CTRL_D, CTRL_R, CTRL_G = (
    "\x06", "\x15", "\x03", "\x04", "\x12", "\x07")
BKSP = "\x7f"
ON = "HELLISH_AUTOSUGGEST=1"

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


CASES = (case_off_by_default, case_on, case_accept, case_enter_plain,
         case_newest, case_edit, case_cursor, case_interrupt,
         case_history_keys, case_search, case_vi, case_escape,
         case_multiline, case_truncate, case_utf8, case_rprompt,
         case_paste, case_max, case_unset, case_user_bind,
         case_postdisplay, case_resize, case_dumb)


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

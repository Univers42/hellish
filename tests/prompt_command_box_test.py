#!/usr/bin/env python3
"""The boxed prompt from issue #132 keeps the cursor where the terminal has it.

The report's configuration builds a two-row, coloured PS1 in a
PROMPT_COMMAND function on every prompt, times commands with a DEBUG trap
that runs `date` in a command substitution, and still had a coloured
RPROMPT clock from an earlier zsh attempt:

    ╭─ javiesan in ~/proyectos/Cursus/Python/PyMod00 on main*
    ╰─ ✔ ❯                                                   20:21:37

Typing on the second row overwrote the prompt: the input started at
column 0, over `╰─ ✔ ❯`, and a long line ran on over rows below. The
cause was fixed in 3.1.0 (the right prompt was drawn inside the line
editor's own prompt string, which made the editor believe the prompt was
nine columns wider than it was). prompt_drift_matrix_test.py pins that
cause with a fixed PS1; this pins the report itself, with its rc as the
user had it, so that the shape that failed in the field keeps working:
a PS1 that PROMPT_COMMAND rewrites before every prompt, a DEBUG trap
that forks, a git segment, and a `✘130` / `✘127` status that changes the
prompt's width from one prompt to the next.

A terminal emulator (pyte) is the judge, never hellish's own width model.
Both line readers are covered: in-process, and forked (HELLISH_RL_FORK=1).

Usage: python3 prompt_command_box_test.py [/path/to/hellish]
"""
import fcntl
import os
import pty
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(HERE, "..", "build", "bin",
                                          "hellish"))
COLS, ROWS = 120, 30
FAILS = []

try:
    import pyte

    class Screen(pyte.Screen):
        """pyte dispatches CSI with parameters; save/restore take none."""

        def save_cursor(self, *_):
            super().save_cursor()

        def restore_cursor(self, *_):
            super().restore_cursor()

    pyte.Stream.csi = dict(pyte.Stream.csi, s="save_cursor",
                           u="restore_cursor")
except ImportError:  # pragma: no cover
    pyte = None

# The issue's ~/.hellish/rc.d/50-prompt.hsh, verbatim but for the
# comments, followed by the RPROMPT line its earlier zsh block had left.
RC = r"""
HX_CMD_START=0
HX_CMD_DUR=""

hx_preexec() {
    [[ "$BASH_COMMAND" == "$PROMPT_COMMAND" ]] && return
    HX_CMD_START=$(date +%s%N)
}
trap 'hx_preexec' DEBUG

hx_git_branch() {
    local b
    b=$(git symbolic-ref --short HEAD 2>/dev/null) || return
    if git diff --quiet --ignore-submodules HEAD 2>/dev/null; then
        printf '%s' "$b"
    else
        printf '%s*' "$b"
    fi
}

hx_precmd() {
    local exit=$?

    HX_CMD_DUR=""
    if (( HX_CMD_START > 0 )); then
        local elapsed=$(( ( $(date +%s%N) - HX_CMD_START ) / 1000000000 ))
        (( elapsed >= 1 )) && HX_CMD_DUR=" took ${elapsed}s"
    fi
    HX_CMD_START=0

    local branch
    branch=$(hx_git_branch)
    [[ -n "$branch" ]] && branch=" on \[\e[35m\]${branch}\[\e[0m\]"

    local icon color
    if [[ $exit -eq 0 ]]; then icon="✔"; color="\[\e[32m\]"
    else icon="✘$exit"; color="\[\e[31m\]"; fi

    PS1="\[\e[90m\]╭─\[\e[0m\] \[\e[32m\]\u\[\e[0m\] in \[\e[36m\]\w\[\e[0m\]${branch}${HX_CMD_DUR} \[\e[90m\]\A\[\e[0m\]
\[\e[90m\]╰─\[\e[0m\] ${color}${icon}\[\e[0m\] ${color}❯\[\e[0m\] "
}

PROMPT_COMMAND="hx_precmd"
RPROMPT='%F{240}%*%f'
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, env, cwd):
        self.screen = Screen(COLS, ROWS)
        self.stream = pyte.ByteStream(self.screen)
        self.buf = bytearray()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd)
            os.environ.clear()
            os.environ.update(env)
            os.execv(SHELL, [SHELL, "-i"])
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", ROWS, COLS, 0, 0))

    def drain(self, cap=3.0, quiet=0.25):
        """Read until the pty has been quiet for `quiet` s; the new bytes."""
        start = len(self.buf)
        last = time.monotonic()
        end = last + cap
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.02)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    break
                if not d:
                    break
                self.buf.extend(d)
                self.stream.feed(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                break
        return bytes(self.buf[start:])

    def wait_for(self, text, cap=8.0):
        """Drain until `text` is on screen (a prompt the hook built)."""
        end = time.monotonic() + cap
        while time.monotonic() < end:
            if any(text in r for r in self.rows()):
                self.drain(1.0, 0.15)
                return True
            self.drain(0.2, 0.05)
        return False

    def until(self, pred, cap=8.0):
        """Drain until pred() holds or `cap` s pass; pred()'s last value.
        A loaded runner may pause mid-redraw, so a check waits for the
        state it expects instead of trusting a quiet gap."""
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.drain(0.2, 0.05)
        return pred()

    def type(self, text):
        """One key at a time, the way a person types."""
        for ch in text:
            os.write(self.fd, ch.encode())
            self.drain(1.0, 0.01)
        self.drain(1.0, 0.15)

    def send(self, data):
        os.write(self.fd, data)
        return self.drain()

    def rows(self):
        return [r.rstrip() for r in self.screen.display]

    def cursor(self):
        return self.screen.cursor.y, self.screen.cursor.x

    def close(self):
        try:
            os.write(self.fd, b"\x15exit\r")
            self.drain(1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def cell(label, home, repo, fork):
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
           "USER": "tester", "HELLISH_NO_BANNER": "1",
           "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
           "ASAN_OPTIONS": "detect_leaks=0"}
    if fork:
        env["HELLISH_RL_FORK"] = "1"
    s = Session(env, repo)
    try:
        run_cell(label, s)
    finally:
        s.close()


def prompt_ok(s, y, icon):
    """Row y-1 is the box's top, row y starts with the status row."""
    rows = s.rows()
    top = rows[y - 1] if y > 0 else ""
    want = "╰─ %s ❯ " % icon
    return (top.startswith("╭─ tester in ~/r on main*")
            and rows[y].startswith(want)), top, rows[y]


def run_cell(label, s):
    ok = s.wait_for("╰─ ✔ ❯")
    check("%s: first prompt drawn" % label, ok, "rows=%r" % s.rows()[:4])
    if not ok:
        return
    y, x = s.cursor()
    good, top, row = prompt_ok(s, y, "✔")
    check("%s: the box is two rows, cursor after ❯" % label,
          good and x == 7, "cursor=%d,%d top=%r row=%r" % (y, x, top, row))
    check("%s: RPROMPT clock at the right of the status row" % label,
          row.rstrip()[-8:].count(":") == 2 and len(row.rstrip()) > 100,
          "row=%r" % row)

    # Typing starts after the prompt, never over it (the report).
    s.type("bbbbbbbbbbbbaaaaaaaaaaaaaaaaaaaaaaaaaa")
    s.until(lambda: s.cursor() == (y, 45))
    y2, x2 = s.cursor()
    row = s.rows()[y2]
    check("%s: typed text follows the prompt on its row" % label,
          y2 == y and row.startswith("╰─ ✔ ❯ bbbbbbbbbbbbaaaa") and x2 == 45,
          "cursor=%d,%d row=%r" % (y2, x2, row))

    # Ctrl-A goes back to the prompt's end on the same row.
    start = len(s.buf)
    s.send(b"\x01")
    s.until(lambda: s.cursor() == (y, 7), 3.0)
    out = bytes(s.buf[start:])
    y3, x3 = s.cursor()
    check("%s: Ctrl-A lands just after ❯, same row" % label,
          (y3, x3) == (y, 7) and b"\x1b[A" not in out,
          "cursor=%d,%d out=%r" % (y3, x3, out[:80]))

    # A line longer than the terminal wraps under the prompt: the prompt
    # row is intact and the rest continues at column 0 of the next row.
    s.send(b"\x05")
    s.type("d" * 110)
    total = 7 + 38 + 110
    s.until(lambda: s.cursor() == (y + 1, total - COLS))
    rows = s.rows()
    y4, x4 = s.cursor()
    check("%s: a wrapped line keeps the prompt and wraps once" % label,
          rows[y].startswith("╰─ ✔ ❯ bbbb") and y4 == y + 1
          and x4 == total - COLS and rows[y + 1] == "d" * (total - COLS),
          "cursor=%d,%d rows=%r" % (y4, x4, rows[y:y + 3]))

    # ^C: the status becomes ✘130, a wider prompt, drawn on fresh rows.
    s.send(b"\x03")
    ok = s.wait_for("╰─ ✘130 ❯")
    s.until(lambda: s.cursor()[1] == 10, 3.0)
    y5, x5 = s.cursor()
    good, top, row = prompt_ok(s, y5, "✘130")
    check("%s: after ^C the ✘130 prompt is whole" % label,
          ok and good and x5 == 10 and y5 >= y + 3,
          "cursor=%d,%d top=%r row=%r" % (y5, x5, top, row))
    s.type("echo " + "z" * 20)
    s.until(lambda: s.cursor() == (y5, 35))
    row = s.rows()[y5]
    check("%s: typing after ✘130 follows the wider prompt" % label,
          row.startswith("╰─ ✘130 ❯ echo zzzz") and s.cursor() == (y5, 35),
          "cursor=%s row=%r" % (s.cursor(), row))

    # Run it, then a missing command: ✘127, still whole.
    s.send(b"\r")
    s.wait_for("╰─ ✔ ❯")
    s.type("ghhgg")
    s.send(b"\r")
    ok = s.wait_for("╰─ ✘127 ❯")
    s.until(lambda: s.cursor()[1] == 10, 3.0)
    y6, x6 = s.cursor()
    good, top, row = prompt_ok(s, y6, "✘127")
    check("%s: after a missing command the ✘127 prompt is whole" % label,
          ok and good and x6 == 10,
          "cursor=%d,%d top=%r row=%r" % (y6, x6, top, row))

    # Up recalls the last command after the prompt; Down comes back to the
    # empty line with the screen as it was.
    before = (s.cursor(), s.rows())
    s.send(b"\x1b[A")
    s.until(lambda: s.cursor() == (y6, 15), 3.0)
    row = s.rows()[y6]
    check("%s: Up recalls the command after the prompt" % label,
          row.startswith("╰─ ✘127 ❯ ghhgg") and s.cursor() == (y6, 15),
          "cursor=%s row=%r" % (s.cursor(), row))
    s.send(b"\x1b[B")
    s.until(lambda: s.cursor() == before[0], 3.0)
    after = (s.cursor(), s.rows())
    check("%s: Down restores the screen exactly" % label,
          before[0] == after[0] and before[1][y6 - 1:y6 + 2]
          == after[1][y6 - 1:y6 + 2],
          "before=%r after=%r" % (before[1][y6], after[1][y6]))

    # Left alone, nothing is repainted anywhere else.
    time.sleep(1.2)
    s.drain(0.5, 0.2)
    check("%s: idle, the prompt stays put" % label,
          s.cursor() == after[0] and s.rows()[y6 - 1:y6 + 1]
          == after[1][y6 - 1:y6 + 1],
          "cursor=%s rows=%r" % (s.cursor(), s.rows()[y6 - 1:y6 + 2]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    if pyte is None:
        print("skip: needs pyte")
        return 0
    if not shutil.which("git"):
        print("skip: needs git")
        return 0
    home = tempfile.mkdtemp(prefix="pcbox-")
    try:
        with open(os.path.join(home, ".hellishrc"), "w") as f:
            f.write(RC)
        repo = os.path.join(home, "r")
        os.mkdir(repo)
        env = dict(os.environ, HOME=home, GIT_CONFIG_NOSYSTEM="1")
        for argv in (["git", "init", "-q", "-b", "main", repo],
                     ["git", "-C", repo, "-c", "user.name=t",
                      "-c", "user.email=t@t", "commit", "-q",
                      "--allow-empty", "-m", "i"]):
            subprocess.run(argv, check=True, env=env)
        with open(os.path.join(repo, ".dirty"), "w") as f:
            f.write("x\n")
        subprocess.run(["git", "-C", repo, "add", ".dirty"], check=True,
                       env=env)
        cell("inproc", home, repo, False)
        cell("fork", home, repo, True)
    finally:
        shutil.rmtree(home, ignore_errors=True)
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

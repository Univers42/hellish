#!/usr/bin/env python3
"""The line is read in the shell process -- what that must not break.

Until 3.2 every interactive line was read in a forked child, and the fork
quietly guaranteed a lot: readline's signal handlers and raw terminal
died with it, a widget's side effects were thrown away, a completion
function's `set -f` did not outlive the TAB. Reading in the shell itself
removes a process per prompt; this file pins everything the child used to
guarantee for free, and what reading in-process now makes possible.

Cases marked (both) also run with HELLISH_RL_FORK=1, the escape hatch that
restores the child for one release, so it cannot rot unnoticed.

   1. ^C at the prompt: ^C echoed, the line not run, $? = 130      (both)
   2. ^C at a dquote> and at a heredoc> continuation                (both)
   3. `trap '' INT`: ^C does nothing and the line survives, as in bash
   4. a background job finishing mid-line, with and without a CHLD trap,
      leaves the line intact
   5. a trapped SIGTERM mid-line: the shell survives, the line goes on
   6. a resize mid-line: the line runs intact
   7. widgets: a `cd` persists (both); a variable persists in-process;
      BUFFER is gone afterwards; $? is the user's across the widget
   8. a widget running `stty sane` leaves the editor working
   9. a widget running `exit 7`: status 7, terminal back to cooked mode
  10. ^D on an empty line exits; on a non-empty line it does not     (both)
  11. typeahead: queued commands run in order, queued Enters draw exactly
      that many prompts                                             (both)
  12. the terminal is cooked again after `exit`, ^D and SIGTERM
  13. closing the terminal leaves no process behind                 (both)
  14. TAB completion leaves IFS, `set -f` and COMP_* as they were
  15. a recalled line edited and abandoned is unchanged in history
  16. the kill ring carries across lines
  17. `zle reset-prompt` from a widget that cds shows the new directory
      without an Enter

Usage: python3 rl_inproc_test.py [/path/to/hellish]
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

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
MARK = b"P$ "
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("" if ok else "\n       " + detail))
    if not ok:
        FAILS.append(name)


class Session:
    def __init__(self, fork=False, argv=None, cwd=None):
        self.home = tempfile.mkdtemp(prefix="hellish_rlin_")
        env = {
            "HOME": self.home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "PS1": "P$ ", "PS2": "C> ",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
            "INPUTRC": "/dev/null",
        }
        if fork:
            env["HELLISH_RL_FORK"] = "1"
        self.raw = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd or self.home)
            os.execvpe((argv or [SHELL])[0],
                       argv or [SHELL, "--norc"], env)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 24, 100, 0, 0))
        self.wait_prompts(1)

    def drain(self, t=0.3):
        end = time.monotonic() + t
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    return False
                if not d:
                    return False
                self.raw += d
        return True

    def wait_prompts(self, n, timeout=10.0):
        """Until n more prompts than at call time were drawn."""
        want = self.raw.count(MARK) + n
        end = time.monotonic() + timeout
        while time.monotonic() < end and self.raw.count(MARK) < want:
            if not self.drain(0.1):
                break
        self.drain(0.2)
        return self.raw.count(MARK) >= want

    def send(self, data, prompts=1, timeout=10.0):
        n0 = len(self.raw)
        os.write(self.fd, data)
        if prompts:
            self.wait_prompts(prompts, timeout)
        else:
            self.drain(0.5)
        return self.raw[n0:]

    def typed(self, data):
        """Keys that do not submit a line."""
        os.write(self.fd, data)
        self.drain(0.3)

    def termios(self):
        return termios.tcgetattr(self.fd)

    def status(self, timeout=5.0):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            self.drain(0.1)
            pid, st = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                return st
        return None

    def close(self):
        try:
            os.kill(self.pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        try:
            os.close(self.fd)
        except OSError:
            pass


def text(b):
    return b.decode("utf-8", "replace")


def case_ctrl_c(fork):
    tag = " (fork)" if fork else ""
    s = Session(fork)
    s.typed(b"echo SHOULD_NOT_RUN")
    out = s.send(b"\x03")
    check("^C at the prompt echoes ^C" + tag, b"^C" in out, repr(out))
    out = s.send(b"echo st=$?\r")
    check("^C: the line did not run and $? is 130" + tag,
          b"st=130" in out and b"SHOULD_NOT_RUN\r\n" not in s.raw,
          repr(s.raw[-300:]))
    s.typed(b'echo "open\r')
    out = s.send(b"\x03")
    out = s.send(b"echo after-dq=$?\r")
    check("^C at a dquote> continuation" + tag, b"after-dq=130" in out,
          repr(out))
    s.typed(b"cat <<EOF\rline\r")
    s.send(b"\x03")
    out = s.send(b"echo after-hd=$?\r")
    check("^C at a heredoc> continuation" + tag,
          b"after-hd=130" in out and b"line\r\nP$" not in out, repr(out))
    s.close()


def case_ignored_int():
    s = Session()
    s.send(b"trap '' INT\r")
    s.typed(b"echo KEPT")
    os.write(s.fd, b"\x03")
    s.drain(0.4)
    out = s.send(b"\r")
    check("trap '' INT: ^C at the prompt does nothing", b"KEPT" in out,
          repr(out))
    s.close()


def case_sigchld():
    s = Session()
    s.send(b"sleep 0.3 &\r")
    s.typed(b"echo half")
    time.sleep(0.8)
    out = s.send(b"way\r")
    check("a job finishing mid-line leaves the line intact",
          b"halfway" in out, repr(out))
    s.send(b"trap 'CH=1' CHLD\r")
    s.send(b"sleep 0.3 &\r")
    s.typed(b"echo again")
    time.sleep(0.8)
    out = s.send(b"-ok\r")
    check("... and with a CHLD trap", b"again-ok" in out, repr(out))
    s.close()


def case_sigterm_trapped():
    s = Session()
    s.send(b"trap 'echo GOT_TERM' TERM\r")
    s.typed(b"echo still")
    os.kill(s.pid, signal.SIGTERM)
    time.sleep(0.5)
    out = s.send(b"-here\r")
    check("a trapped SIGTERM mid-line: the line goes on",
          b"still-here" in out, repr(out))
    check("... and the shell is alive", s.status(0.2) is None)
    s.close()


def case_resize():
    s = Session()
    s.typed(b"echo resized")
    fcntl.ioctl(s.fd, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 60, 0, 0))
    os.kill(s.pid, signal.SIGWINCH)
    time.sleep(0.4)
    out = s.send(b"-fine\r")
    check("a resize mid-line: the line runs intact",
          b"resized-fine" in out, repr(out))
    s.close()


# Key sequences in readline's own escape syntax (\e), as zle_test.py
# writes them: zsh's caret form (^X) is not what readline parses.
WIDGETS = (b"wcd() { cd /; WV=set; false; }; zle -N wcd; "
           b"bindkey '\\ez' wcd\r")


def case_widgets(fork):
    tag = " (fork)" if fork else ""
    s = Session(fork)
    s.send(WIDGETS)
    s.send(b"true\r")
    s.typed(b"\x1bz")
    out = s.send(b'echo "pwd=$PWD st=$? wv=${WV-unset} buf=${BUFFER-unset}"'
                 b"\r")
    check("a widget's cd persists" + tag, b"pwd=/ " in out, repr(out))
    check("$? is the user's across the widget" + tag, b"st=0" in out,
          repr(out))
    check("BUFFER does not outlive the widget" + tag, b"buf=unset" in out,
          repr(out))
    if fork:
        check("a widget's variable dies with the child (fork)",
              b"wv=unset" in out, repr(out))
    else:
        check("a widget's variable persists", b"wv=set" in out, repr(out))
    s.close()


def case_widget_stty():
    s = Session()
    s.send(b"ws() { stty sane; WS=ran; }; zle -N ws; bindkey '\\ey' ws\r")
    s.typed(b"echo edit")
    s.typed(b"\x1by")
    s.typed(b"\x7f\x7f\x7f\x7fITED $WS")
    out = s.send(b"\r")
    check("a widget running stty leaves the editor working",
          b"\rITED ran\r\n" in out, repr(out))
    s.close()


def case_widget_exit():
    s = Session()
    s.send(b"wx() { exit 7; }; zle -N wx; bindkey '\\ex' wx\r")
    os.write(s.fd, b"\x1bx")
    st = s.status()
    check("a widget running `exit 7` exits 7",
          st is not None and os.WIFEXITED(st) and os.WEXITSTATUS(st) == 7,
          "status %r" % st)
    s.close()


def case_ctrl_d(fork):
    tag = " (fork)" if fork else ""
    s = Session(fork)
    s.typed(b"echo abc")
    os.write(s.fd, b"\x04")
    s.drain(0.4)
    check("^D on a non-empty line does not exit" + tag,
          s.status(0.3) is None)
    s.send(b"\x15")
    os.write(s.fd, b"\x04")
    st = s.status()
    check("^D on an empty line exits" + tag, st is not None,
          "status %r" % st)
    s.close()


def case_typeahead(fork):
    tag = " (fork)" if fork else ""
    s = Session(fork)
    n0 = s.raw.count(MARK)
    os.write(s.fd, b"echo A1\recho A2\recho A3\r")
    s.wait_prompts(3)
    tail = text(s.raw)
    order = [tail.find("\rA%d\r\n" % i) for i in (1, 2, 3)]
    check("queued commands run in order" + tag,
          all(o >= 0 for o in order) and order == sorted(order),
          repr(tail[-200:]))
    n1 = s.raw.count(MARK)
    os.write(s.fd, b"\r" * 12)
    s.wait_prompts(12)
    s.drain(0.6)
    check("12 queued Enters draw exactly 12 prompts" + tag,
          s.raw.count(MARK) - n1 == 12,
          "%d" % (s.raw.count(MARK) - n1))
    s.close()


def cooked_after(action):
    """Run the shell under sh, act, then have sh print the terminal
    modes: they must be the cooked ones readline found."""
    wrap = ["/bin/sh", "-c", "'%s' --norc; stty -a; echo END_STTY" % SHELL]
    s = Session(argv=wrap)
    action(s)
    end = time.monotonic() + 8
    while time.monotonic() < end and b"END_STTY" not in s.raw:
        if not s.drain(0.1):
            break
    out = text(s.raw)
    s.close()
    stty = out[out.rfind("speed"):] if "speed" in out else out[-400:]
    return (" icanon" in stty and " echo " in stty
            and "-icanon" not in stty), stty[-200:]


def case_terminal_restored():
    for name, act in (
            ("exit", lambda s: os.write(s.fd, b"exit\r")),
            ("^D", lambda s: os.write(s.fd, b"\x04")),
            ("SIGTERM at the prompt", lambda s: (
                time.sleep(0.3),
                os.system("pkill -TERM -P %d >/dev/null 2>&1" % s.pid)))):
        ok, detail = cooked_after(act)
        check("the terminal is cooked after " + name, ok, repr(detail))


def case_hangup(fork):
    tag = " (fork)" if fork else ""
    s = Session(fork)
    pid = s.pid
    os.close(s.fd)
    time.sleep(1.0)
    try:
        wpid, _ = os.waitpid(pid, os.WNOHANG)
    except ChildProcessError:
        wpid = pid
    alive = False
    if not wpid:
        try:
            os.kill(pid, 0)
            alive = True
        except OSError:
            alive = False
    check("closing the terminal leaves no shell behind" + tag, not alive)
    try:
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
    except (OSError, ChildProcessError):
        pass


def case_completion_sandbox():
    s = Session()
    s.send(b"complete -W 'alpha alpine' foo\r")
    s.send(b"set -f\r")
    before = s.send(b'printf "[%s]" "$IFS" "$-" "${COMP_LINE-u}" '
                    b'"${COMPREPLY-u}"; echo\r')
    s.typed(b"foo al\t")
    s.typed(b"\t")
    s.send(b"\x15\r")
    after = s.send(b'printf "[%s]" "$IFS" "$-" "${COMP_LINE-u}" '
                   b'"${COMPREPLY-u}"; echo\r')
    pick = lambda b: text(b).split("\r\n")[1] if "\r\n" in text(b) else ""
    check("TAB completion leaves IFS, set -f and COMP_* as they were",
          pick(before) != "" and pick(before) == pick(after),
          "before %r after %r" % (pick(before), pick(after)))
    s.close()


def case_history_revert():
    s = Session()
    s.send(b"echo one\r")
    s.send(b"echo two\r")
    s.typed(b"\x1b[A\x1b[AXX")
    s.typed(b"\x1b[B\x1b[B")
    s.send(b"echo three\r")
    out = s.send(b"history\r")
    check("a recalled line edited and abandoned is unchanged",
          b"echo one\r" in out and b"echo oneXX" not in out, repr(out))
    s.close()


def case_kill_ring():
    s = Session()
    s.typed(b"echo keptline")
    s.typed(b"\x15")
    s.send(b"\r")
    s.typed(b"\x19")
    out = s.send(b"\r")
    check("the kill ring carries across lines", b"keptline" in out,
          repr(out))
    s.close()


def case_reset_prompt():
    dest = tempfile.mkdtemp(prefix="rp_dest_")
    s = Session()
    s.send(b"PS1='P$ \\w> '\r")
    s.send(("go() { cd %s; zle reset-prompt; }; zle -N go; "
            "bindkey '\\eg' go\r" % dest).encode())
    n0 = len(s.raw)
    os.write(s.fd, b"\x1bg")
    s.drain(0.8)
    shown = s.raw[n0:]
    check("zle reset-prompt shows the new directory at once",
          os.path.basename(dest).encode() in shown, repr(shown[-200:]))
    s.close()


def main():
    if not os.access(SHELL, os.X_OK):
        print("error: no shell at %s" % SHELL)
        return 2
    for fork in (False, True):
        case_ctrl_c(fork)
        case_widgets(fork)
        case_ctrl_d(fork)
        case_typeahead(fork)
        case_hangup(fork)
    case_ignored_int()
    case_sigchld()
    case_sigterm_trapped()
    case_resize()
    case_widget_stty()
    case_widget_exit()
    case_terminal_restored()
    case_completion_sandbox()
    case_history_revert()
    case_kill_ring()
    case_reset_prompt()
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

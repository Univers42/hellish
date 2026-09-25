"""An interactive hellish on a pty, driven by what it PRINTS, not by a clock.

Most pty tests here sleep a fixed time after every keystroke, long enough
for the slowest CI runner -- so every step costs that much on every runner,
and a test with thirty steps waits out thirty worst cases. This waits for
the text it needs and moves on the moment it arrives; the timeout is only
the failure bound. A shell that answers in 3 ms makes a step cost 3 ms.

    from ptyexpect import Tty          # tests add tests/helpers to sys.path
    t = Tty(shell)                      # PS1 is 'P$ ', HOME is a temp dir
    t.expect(b"P$ ")                    # consume through the first prompt
    t.send("echo hi\r")
    assert t.expect(b"hi\r\n")
    status = t.close()                  # a wait status: WIFSIGNALED works

Lives in tests/helpers/ so tests/pty_suite.sh, which runs every tests/*.py,
does not mistake it for a test.
"""
import os
import pty
import select
import shutil
import tempfile
import time


class Tty:
    """One interactive shell on a pty, in a throwaway HOME."""

    def __init__(self, shell, rc="PS1='P$ '\n", env=None, path=None,
                 cols=100, rows=30):
        self.home = tempfile.mkdtemp(prefix="ptyexpect-")
        with open(os.path.join(self.home, ".hellishrc"), "w") as f:
            f.write(rc)
        base = {"HOME": self.home,
                "PATH": path or os.environ.get("PATH", "/usr/bin:/bin"),
                "TERM": "dumb", "LANG": "C.UTF-8", "USER": "tester",
                "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
                "HELLISH_NO_UPDATE_CHECK": "1",
                "ASAN_OPTIONS": "detect_leaks=0"}
        base.update(env or {})
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(self.home)
            os.environ.clear()
            os.environ.update(base)
            os.execv(shell, [shell, "-i"])
            os._exit(127)
        try:
            import fcntl
            import struct
            import termios
            fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                        struct.pack("HHHH", rows, cols, 0, 0))
        except OSError:
            pass
        self.out = b""
        self.pos = 0

    def _read(self, timeout):
        r, _, _ = select.select([self.fd], [], [], timeout)
        if not r:
            return True
        try:
            d = os.read(self.fd, 65536)
        except OSError:
            return False
        if not d:
            return False
        self.out += d
        return True

    def expect(self, needle, timeout=10.0):
        """Read until `needle` appears after what earlier expects consumed,
        then consume through it. False on timeout or when the shell is
        gone -- the caller turns that into a named failure."""
        if isinstance(needle, str):
            needle = needle.encode()
        end = time.time() + timeout
        while self.out.find(needle, self.pos) < 0 and time.time() < end:
            if not self._read(0.05):
                break
        at = self.out.find(needle, self.pos)
        if at < 0:
            return False
        self.pos = at + len(needle)
        return True

    def since(self, mark):
        """Everything printed after byte offset `mark`, CRs dropped."""
        return self.out[mark:].replace(b"\r", b"")

    def send(self, s):
        os.write(self.fd, s.encode() if isinstance(s, str) else s)

    def _reap(self, timeout):
        end = time.time() + timeout
        while time.time() < end:
            pid, st = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                return st
            self._read(0.05)
        return None

    def close(self, timeout=5.0):
        """Ask the shell to leave, and return its wait status. `exit` on
        its own first: an interrupt sent right before it makes readline
        drop the type-ahead, and the shell would sit at its prompt. Only a
        shell still busy gets the interrupt, then `exit` again."""
        status = None
        for keys in (b"exit\r", b"\x03", b"exit\r"):
            try:
                os.write(self.fd, keys)
            except OSError:
                pass
            status = self._reap(timeout / 3)
            if status is not None:
                break
        if status is None:
            os.kill(self.pid, 9)
            status = os.waitpid(self.pid, 0)[1]
        try:
            os.close(self.fd)
        except OSError:
            pass
        shutil.rmtree(self.home, ignore_errors=True)
        return status

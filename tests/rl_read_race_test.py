#!/usr/bin/env python3
"""Regression gate: a ^C between "a key is ready" and read() loses no key.

prompt_interrupt_test.py failed now and then under load: ^C inside a ^R
search, then `echo ISR_$((1+1))` ran as `cho ISR_2` -- the new line's
first key had gone to the search the ^C was meant to end, five seconds
after that ^C, which had seemingly done nothing.

The reader (rl_getc.c) waits in pselect, which lets signals in
atomically, and reads the key once pselect reports one. A ^C can be
processed by the line discipline between the two, and then two things
happen in this order: SIGINT is sent -- readline's handler runs before
read() is entered, so no EINTR is coming -- and the input queue is
flushed, taking the key pselect saw with it. A BLOCKING read() then slept
on an empty queue until the next key arrived, and handed that key to the
line the ^C had ended.

Timing makes it rare; this file makes it certain. A small LD_PRELOAD shim
stalls the shell's read() right there -- after pselect, before the real
read -- for the one key that follows a chosen byte, and says so through a
marker file; the ^C is sent only once the shell is known to be in the
gap. Before the fix every scenario below hangs until the next key and
eats it; after it, the ^C ends the line at once and the next command runs
whole.

  prompt    a half-typed line, ^C in the gap, then a command
  isearch   ^C in the gap inside a ^R search (the reported failure)

Each runs with the in-process reader and with the forked one
(HELLISH_RL_FORK=1), whose child inherits the same read.

The fix reads the keys from a second, non-blocking open of the terminal
(rl_keyfd.c), so two things about it are checked too, with no shim:

  hygiene   no program the shell runs inherits that descriptor
  nodelay   fd 0 left O_NONBLOCK by a program is blocking again at the
            next command, as bash makes it before every readline()

The shim scenarios skip (exit 0 for them, says so) where the shim cannot
take effect: no C compiler, or a binary whose own read() comes first
(clang links ASan's interceptors into the executable). CI's pty job
builds with gcc. Linux only: /proc and the line discipline under test.

Usage: python3 rl_read_race_test.py /path/to/hellish
"""
import os
import pty
import select
import shutil
import subprocess
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []
MARK = b"#> "
STALL = 1.5
SHIM_C = r"""
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Stall the one 1-byte terminal read that follows a read of the byte
   HX_STALL_AFTER, after creating HX_STALL_MARK, for 1.5s (STALL) -- the
   whole of it, a signal in the middle included. */
static ssize_t	(*real_read)(int, void *, size_t);
static int		armed;

static void	stall(void)
{
	struct timespec	ts;
	const char		*mark;

	armed = 0;
	mark = getenv("HX_STALL_MARK");
	if (mark)
		close(open(mark, O_WRONLY | O_CREAT, 0644));
	ts.tv_sec = 1;
	ts.tv_nsec = 500000000L;
	while (nanosleep(&ts, &ts) < 0)
		;
}

ssize_t	read(int fd, void *buf, size_t n)
{
	const char	*after;
	ssize_t		r;

	if (!real_read)
		real_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT,
				"read");
	if (armed && n == 1 && isatty(fd))
		stall();
	r = real_read(fd, buf, n);
	after = getenv("HX_STALL_AFTER");
	if (after && r == 1 && n == 1 && isatty(fd)
		&& ((unsigned char *)buf)[0] == (unsigned char)atoi(after))
		armed = 1;
	return (r);
}
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def build_shim(base):
    cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if not cc:
        return None
    src = os.path.join(base, "stall.c")
    lib = os.path.join(base, "stall.so")
    with open(src, "w") as f:
        f.write(SHIM_C)
    r = subprocess.run([cc, "-shared", "-fPIC", "-o", lib, src, "-ldl"],
                       capture_output=True)
    return lib if r.returncode == 0 else None


class Session:
    def __init__(self, home, shim, mark, extra):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
            "PS1": "P#> ", "HISTFILE": os.path.join(home, "hist"),
            "HELLISH_BANNER": "0", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "LD_PRELOAD": shim, "HX_STALL_MARK": mark,
            "ASAN_OPTIONS": "detect_leaks=0:verify_asan_link_order=0",
        }
        env.update(extra)
        self.mark = mark
        self.buf = bytearray()
        self.at = 0
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(home)
            os.execve(SHELL, [SHELL, "--norc"], env)
            os._exit(127)
        self.prompt(10.0)

    def pump(self, secs):
        r, _, _ = select.select([self.fd], [], [], secs)
        if r:
            try:
                d = os.read(self.fd, 65536)
            except OSError:
                return
            self.buf.extend(d)

    def until(self, pred, cap):
        end = time.monotonic() + cap
        while not pred() and time.monotonic() < end:
            self.pump(0.05)
        return pred()

    def prompt(self, cap):
        """Wait for a prompt drawn after self.at; move self.at past it."""
        ok = self.until(lambda: MARK in self.buf[self.at:], cap)
        if ok:
            self.at = self.buf.index(MARK, self.at) + len(MARK)
        return ok

    def send(self, data):
        os.write(self.fd, data)

    def run(self, line, token, cap=5.0):
        start = self.at
        self.send(line)
        ok = self.until(lambda: token in self.buf[start:], cap)
        if ok:
            self.at = self.buf.index(token, start) + len(token)
            self.prompt(cap)
        return ok

    def in_gap(self, cap=5.0):
        end = time.monotonic() + cap
        while time.monotonic() < end:
            if os.path.exists(self.mark):
                return True
            self.pump(0.02)
        return False

    def tail(self):
        return bytes(self.buf[-200:])

    def close(self):
        try:
            os.kill(self.pid, 9)
        except OSError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass


def descriptors(base):
    """The second descriptor stays the shell's own, and fd 0 is put back
    to blocking before a read, as bash does."""
    home = tempfile.mkdtemp(dir=base)
    s = Session(home, "", os.path.join(home, "unused"), {})
    try:
        s.run(b"python3 -c 'import os; d = \"/proc/self/fd/\"; print("
              b"\"TTYFD_%s_\" % [f for f in os.listdir(d) if int(f) > 2 and"
              b" os.path.realpath(d + f) == os.ttyname(0)])'\r", b"_\r\n")
        got = bytes(s.buf[s.buf.rindex(b"TTYFD_"):]).split(b"_\r")[0]
        check("hygiene: a program the shell runs sees the terminal on 0-2 only",
              got == b"TTYFD_[]", repr(got))
        s.run(b"python3 -c 'import os,fcntl; fcntl.fcntl(0, fcntl.F_SETFL,"
              b" fcntl.fcntl(0, fcntl.F_GETFL) | os.O_NONBLOCK)';"
              b" echo SET_$((2+2))\r", b"SET_4")
        s.run(b"python3 -c 'import os,fcntl; print(\"NB_%d\" % bool(fcntl."
              b"fcntl(0, fcntl.F_GETFL) & os.O_NONBLOCK))'\r", b"NB_")
        after = bytes(s.buf[s.buf.rindex(b"NB_"):][:4])
        check("nodelay: fd 0 left non-blocking is blocking at the next command",
              after == b"NB_0", repr(after))
    finally:
        s.close()


def scenario(label, shim, base, extra, trigger, keys, what):
    """`keys` is typed in one write; the shell stalls in the gap before
    the key after `trigger`, and ^C is sent while it is there."""
    home = tempfile.mkdtemp(dir=base)
    mark = os.path.join(home, "in-gap")
    env = dict(extra, HX_STALL_AFTER=str(trigger))
    s = Session(home, shim, mark, env)
    try:
        s.run(b"echo one_$((0+1))\r", b"one_1")
        start = s.at
        s.send(keys)
        if not s.in_gap():
            return None
        t0 = time.monotonic()
        s.send(b"\x03")
        # the shim holds the shell for STALL; without the fix nothing
        # comes after it either, until another key is typed
        prompt = s.prompt(STALL + 1.5)
        dt = time.monotonic() - t0
        check("%s: %s: ^C in the gap ends the line, no key needed"
              % (label, what), prompt,
              "no new prompt %.1fs after ^C: %r" % (dt, s.tail()))
        check("%s: %s: ^C is echoed once" % (label, what),
              s.buf[start:].count(b"^C") == 1, repr(s.tail()))
        ok = s.run(b"echo NEXT_$((2+3))\r", b"NEXT_5")
        check("%s: %s: the next command runs whole" % (label, what),
              ok and b"command not found" not in s.buf[start:],
              repr(s.tail()))
        check("%s: %s: nothing typed before ^C ran" % (label, what),
              b"LEAK_42" not in s.buf[start:], repr(s.tail()))
        return True
    finally:
        s.close()


def main():
    if not os.path.exists(SHELL):
        print("FAIL shell not found: " + SHELL)
        return 1
    if not sys.platform.startswith("linux"):
        print("skip: Linux only (/proc, and the line discipline under test)")
        return 0
    base = tempfile.mkdtemp(prefix="hellish_rl_read_race_")
    descriptors(base)
    shim = build_shim(base)
    if not shim:
        print("skip: no C compiler: the stall shim cannot be built")
        return finish(base)
    cells = (("in-process", {}), ("forked", {"HELLISH_RL_FORK": "1"}))
    for label, extra in cells:
        for trigger, keys, what in (
                (ord("x"), b"x; echo LEAK_$((6*7))", "prompt"),
                (0x12, b"\x12ech", "isearch")):
            r = scenario(label, shim, base, extra, trigger, keys, what)
            if r is None:
                print("skip: the shell never stalled in the shim -- its own "
                      "read() comes first (clang ASan?), so the gap cannot "
                      "be held open")
                return finish(base)
    return finish(base)


def finish(base):
    shutil.rmtree(base, ignore_errors=True)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

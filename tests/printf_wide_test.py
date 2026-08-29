#!/usr/bin/env python3
"""Regression test: printf honours wide field widths -- issue #73.

    printf "%10000000s" c | tr " " "a"

produced 4095 bytes instead of 10000000, for ANY width, silently. There were
TWO caps stacked on each other:

  * pf_conv rendered every conversion into a fixed char[4096];
  * pf_build_spec ALSO clamped the width to 30000, with a comment claiming
    that was "well above the 4096-byte render buffer, so output is
    unaffected" -- exactly backwards, since 30000 > 4096. The clamp hid
    nothing and the buffer truncated everything.

The render buffer is now sized from the SPEC (width and precision are known
before any conversion runs), so there is one measurement and one format. That
matters beyond tidiness: asking snprintf for the length means formatting
twice, and pf_num/pf_unum report conversion errors as a side effect, so a
second pass would print every "invalid number" diagnostic twice.

The stack buffer stays the fast path -- anything under 4096 bytes, which is
every conversion anyone actually writes, allocates nothing.

The issue also reports a "prompt tty issue". That half did NOT reproduce: with
the truncation fixed, hellish and bash place the prompt identically after
unterminated output, and a PS1 opening with \\n still starts on a fresh line
at every width. It was the short output plus terminal wrapping. The prompt
checks below stay so that a real regression there would be caught.

Usage: python3 printf_wide_test.py [/path/to/hellish]
"""
import os
import pty
import select
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def out_of(sh, script):
    return subprocess.run([sh, "-c", script], capture_output=True,
                          env=ENV, timeout=120).stdout


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    # The reported widths, plus the boundary where the old buffer gave up.
    for w in (10, 4095, 4096, 4097, 20000, 100000, 10000000):
        got = len(out_of(SHELL, "printf '%%%ds' c" % w))
        check("printf '%%%ds' emits %d bytes" % (w, w), got == w,
              "got %d -- 4095 means the render buffer is fixed again" % got)

    # Left-justified and precision take the same path.
    check("a negative (left-justified) width is honoured",
          len(out_of(SHELL, "printf '%-50000s' c")) == 50000,
          "got %d" % len(out_of(SHELL, "printf '%-50000s' c")))
    check("a wide precision is honoured",
          len(out_of(SHELL, "printf '%.50000d' 7")) == 50000,
          "got %d" % len(out_of(SHELL, "printf '%.50000d' 7")))
    check("a wide numeric width is honoured",
          len(out_of(SHELL, "printf '%50000d' 7")) == 50000,
          "got %d" % len(out_of(SHELL, "printf '%50000d' 7")))

    # Ordinary widths must be untouched -- this is the allocation-free path.
    check("normal conversions are unchanged",
          out_of(SHELL, "printf '[%5s][%-5s][%05d][%.2f]' ab cd 42 3.14159")
          == b"[   ab][cd   ][00042][3.14]",
          "got %r" % out_of(SHELL,
                            "printf '[%5s][%-5s][%05d][%.2f]' ab cd 42 3.14159"))

    # An invalid number must still report ONCE, not twice -- the reason the
    # size is computed from the spec rather than by formatting twice.
    p = subprocess.run([SHELL, "-c", "printf '%d' notanumber"],
                       capture_output=True, text=True, env=ENV, timeout=30)
    check("a bad number is diagnosed exactly once",
          p.stderr.count("invalid number") == 1,
          "stderr=%r" % p.stderr.strip()[:160])

    # Byte-for-byte against the pinned oracle, which is the real contract.
    if os.path.exists(ORACLE):
        for script in ("printf '%20000s' c", "printf '%-4096s|' x",
                       "printf '%.9000d' 3"):
            check("matches bash: %s" % script,
                  out_of(SHELL, script) == out_of(ORACLE, script),
                  "lengths %d vs %d" % (len(out_of(SHELL, script)),
                                        len(out_of(ORACLE, script))))
    else:
        print("skip (no oracle at %s)" % ORACLE)

    # The prompt half: a PS1 opening with \n must still start on a fresh line
    # after a large unterminated write.
    env = dict(ENV, TERM="dumb", HOME="/tmp")
    env.pop("PS1", None)
    env.pop("PROMPT", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL, "-i"], env)
        os._exit(1)
    buf = b""

    def pump(t):
        nonlocal buf
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                continue
            try:
                d = os.read(fd, 1 << 20)
            except OSError:
                break
            if not d:
                break
            buf += d
    pump(1.0)
    os.write(fd, br"""PS1='\nMARK> '""" + b"\n")
    pump(0.6)
    buf = b""
    os.write(fd, b"printf '%20000s' c | tr ' ' a\n")
    pump(3.0)
    os.write(fd, b"exit\n")
    pump(0.5)
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    text = buf.decode("utf-8", "replace")
    check("20000 bytes actually reach the terminal",
          text.count("a") >= 20000, "counted %d" % text.count("a"))
    i = text.find("MARK>", text.find("aaa"))
    check("a PS1 opening with a newline still starts on a fresh line",
          i > 0 and "\n" in text[max(0, i - 6):i],
          "chars before prompt: %r" % text[max(0, i - 6):i])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

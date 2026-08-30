#!/usr/bin/env python3
"""Regression test: the zsh-style `%` prompt syntax -- issue #69.

#69 asks whether

    PROMPT="%K{white}%F{black} %? %f%k%K{green} %n@%m %k..."

would be nicer than the backslash form, and the answer taken was "both",
not "one instead of the other".

So this is a FRONTEND, not a second prompt engine: every `%` escape is
rewritten into the backslash language and the existing renderer does the
work. That matters because the alternative -- a parallel renderer -- means
two width models, two expanders and two sets of extensions, which drift.
The test therefore checks that both spellings of the same prompt produce
the SAME bytes, not merely that each produces something.

Opt-in is by variable, not by sniffing: PROMPT selects the zsh reader, PS1
the bash one. Sniffing would have to guess what a bare `%` means, and `%`
is an ordinary character in a bash prompt -- anyone with a literal percent
in their PS1 would have watched it vanish.

Usage: python3 prompt_zsh_test.py [/path/to/hellish]
"""
import os
import pty
import select
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def paint(setup, cmds):
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", TERM="dumb")
    for v in ("PS1", "PROMPT"):
        env.pop(v, None)
    pid, fd = pty.fork()
    if pid == 0:
        # --norc: pin the config. An inherited ~/.hellishrc can set PS1 or
        # define names, and quietly decide what this test sees.
        os.execve(SHELL, [SHELL, "-i", "--norc"], env)
        os._exit(1)
    out = b""

    def pump(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.15)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
    pump(1.0)
    for s in setup:
        os.write(fd, s.encode() + b"\n")
        pump(0.5)
    out = b""
    for c in cmds:
        os.write(fd, c.encode() + b"\n")
        pump(0.6)
    os.write(fd, b"exit\n")
    pump(0.3)
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    return out.decode("utf-8", "replace")


def marks(out, tag):
    return [l.strip() for l in out.splitlines() if tag in l]


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    # The property that keeps the two syntaxes from drifting: same prompt,
    # two spellings, identical output.
    a = paint(["PROMPT='<%n@%m %~>'"], ["true"])
    b = paint([r"""PS1='<\u@\h \w>'"""], ["true"])
    ma, mb = marks(a, "<"), marks(b, "<")
    check("%n@%m %~ renders exactly like \\u@\\h \\w",
          bool(ma) and bool(mb) and ma[0] == mb[0],
          "zsh=%r bash=%r" % (ma[:1], mb[:1]))

    # %? is the reason #69 was filed; it must track the real status.
    o = paint(["PROMPT='[st=%?]'"], ["sh -c 'exit 5'", "true"])
    got = marks(o, "st=")
    check("%? tracks the real exit status",
          any("st=5" in l for l in got) and any("st=0" in l for l in got),
          "got %r" % got[:4])

    # %% is a literal percent, and an unknown escape is left visible rather
    # than silently eaten.
    o = paint(["PROMPT='[a=100%% b=%q]'"], ["true"])
    check("%% is a literal percent and %q stays visible",
          any("a=100% b=%q" in l for l in marks(o, "a=")),
          "got %r" % marks(o, "a=")[:2])

    # Colour goes through the same equivalence. Note the reset lands BETWEEN
    # "ab" and ">", so searching for the substring "ab>" finds nothing even
    # when the prompt is perfectly correct -- an earlier version of this
    # check did that and reported a failure against working code. Comparing
    # the two spellings avoids guessing what the bytes should look like.
    a = paint(["PROMPT='%F{green}ab%f>'"], ["true"])
    b = paint([r"""PS1='\[\e[38;5;2m\]ab\[\e[0m\]>'"""], ["true"])
    ma, mb = marks(a, "ab"), marks(b, "ab")
    check("%F{colour}/%f renders exactly like the \\[\\e[..m\\] form",
          bool(ma) and ma == mb, "zsh=%r bash=%r" % (ma[:1], mb[:1]))
    check("...and it really emits an SGR sequence",
          bool(ma) and "\x1b[" in ma[0], "got %r" % ma[:1])

    # PS1 must keep working untouched when PROMPT is unset, and PROMPT wins
    # when both are set (that is the opt-in).
    o = paint([r"""PS1='[bash-only]'"""], ["true"])
    check("PS1 alone still works", bool(marks(o, "bash-only")),
          "got %r" % marks(o, "bash-only")[:2])
    o = paint([r"""PS1='[PS1]'""", "PROMPT='[PROMPT]'"], ["true"])
    check("PROMPT wins when both are set",
          bool(marks(o, "[PROMPT]")) and not marks(o, "[PS1]"),
          "got %r" % (marks(o, "[")[:3],))

    # hellish's own badges have no zsh spelling, so they stay reachable by
    # their backslash names from inside a PROMPT.
    o = paint([r"""PROMPT='%n[\S]'"""], ["sh -c 'exit 4'"])
    check("hellish badges still work inside PROMPT",
          any("4" in l for l in marks(o, "[")),
          "got %r" % marks(o, "[")[:3])

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

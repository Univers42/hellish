#!/usr/bin/env python3
"""Regression test: special parameters expand in PS1 -- issue #69.

The report is "the exit code is not 0 just delete it": a prompt showing a
permanent 0 no matter what the last command returned. There were TWO causes,
and only fixing one would have left the complaint standing.

1. The reporter's PS1 was DOUBLE-quoted:

       PS1="...\\[\\e[30;47m\\] $? \\[\\e[0m\\]..."

   so `$?` was expanded once, at assignment, and the literal `0` was baked in
   forever. `echo "$?"` in the same transcript correctly printed 2, which is
   what made it look like the prompt was lying rather than the quoting.

2. Single quotes did not help either, because `ps1_dollar()` guarded on
   is_var_name_p1() -- isalpha or '_' -- so '?' fell through to LITERAL text
   and `PS1='[$?] '` rendered the two characters `$?` on screen, forever.

   `${?}` was always correct, because the braced path routes into the real
   parameter expander. So the shell could already produce the value; the
   unbraced reader just refused to ask for it.

bash re-expands PS1 on every prompt, so the reference behaviour is checked
against the pinned oracle in the same pty harness rather than asserted from
memory.

Usage: python3 ps1_specials_test.py [/path/to/hellish]
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


def prompts(sh, args, setup, cmds):
    """Drive a real interactive shell; return everything it painted."""
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", TERM="dumb")
    env.pop("PS1", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(sh, [sh] + list(args), env)
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


def seen(out, tag):
    return [l.strip() for l in out.splitlines() if tag in l]


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    # The exact complaint: does the prompt track the real exit status?
    out = prompts(SHELL, ["-i"], ["PS1='[st=$?] '"],
                  ["sh -c 'exit 7'", "true"])
    lines = seen(out, "st=")
    check("PS1 '$?' tracks the real exit status",
          any("st=7" in l for l in lines) and any("st=0" in l for l in lines),
          "got %r -- a frozen or literal value is the bug" % lines[:4])

    # The braced form was always right; it must stay right.
    out = prompts(SHELL, ["-i"], ["PS1='[st=${?}] '"], ["sh -c 'exit 3'"])
    check("PS1 '${?}' still tracks it",
          any("st=3" in l for l in seen(out, "st=")),
          "got %r" % seen(out, "st=")[:3])

    # Never render the literal characters again.
    out = prompts(SHELL, ["-i"], ["PS1='[st=$?] '"], ["true"])
    check("PS1 never shows a literal '$?'",
          not any("st=$?" in l for l in seen(out, "st=")),
          "got %r" % seen(out, "st=")[:3])

    # The other specials on the same path.
    out = prompts(SHELL, ["-i"], ["PS1='[p=$$] '"], ["true"])
    vals = [l for l in seen(out, "p=") if "p=$$" not in l]
    check("PS1 '$$' expands to a pid", bool(vals) and any(
        c.isdigit() for l in vals for c in l), "got %r" % seen(out, "p=")[:3])

    # A NAME must still work -- the new dispatch must not shadow it.
    out = prompts(SHELL, ["-i"], ["MYV=hello", "PS1='[v=$MYV] '"], ["true"])
    check("a plain $NAME still expands",
          any("v=hello" in l for l in seen(out, "v=")),
          "got %r" % seen(out, "v=")[:3])

    # And an unknown '$' must stay literal rather than eating a character.
    out = prompts(SHELL, ["-i"], ["PS1='[a=$ b] '"], ["true"])
    check("a bare '$' is left alone",
          any("a=$ b" in l for l in seen(out, "a=")),
          "got %r" % seen(out, "a=")[:3])

    # Reference: this is what bash does, checked rather than remembered.
    oracle = os.path.expanduser(os.environ.get(
        "HELLISH_ORACLE", "~/bash-5.3.9/bin/bash"))
    if os.path.exists(oracle):
        out = prompts(oracle, ["--norc", "-i"], ["PS1='[st=$?] '"],
                      ["sh -c 'exit 7'", "true"])
        lines = seen(out, "st=")
        check("(reference) bash behaves the same way",
              any("st=7" in l for l in lines),
              "oracle disagreed: %r" % lines[:4])
    else:
        print("skip (no oracle at %s)" % oracle)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""#72 phase 2.3: HELLISH_PRECMD_FUNCS / HELLISH_PREEXEC_FUNCS.

Two hook points a plugin can attach to without taking them away from any
other plugin:

    HELLISH_PRECMD_FUNCS    run before each primary prompt
    HELLISH_PREEXEC_FUNCS   run before the typed line executes, with the
                            line as $1

The composability is the whole point, and it is why this is an ARRAY of
function names rather than a string of code. `trap DEBUG` is the shape this
replaces: it holds exactly one handler, so the second plugin to install one
silently removes the first, and neither can tell. Two names in an array
both run, and the test that matters is the one that puts two in.

$? is the other half. A hook that runs a command clobbers the status the
user's own command left behind, so a prompt with a `$?` badge would report
the hook's result forever -- exactly the frozen-status shape #69 was.
PROMPT_COMMAND already brackets this correctly; the hooks reuse the bracket.

And non-interactive shells must not run either. That is the same rule
~/.hellishrc has had all along: a script that inherits the developer's
hooks is a test suite that fails on one machine.

Usage: python3 rc_hooks_test.py [/path/to/hellish]
"""
import os
import pty
import select
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def session(rc_body, cmds):
    """Interactive shell with rc_body as its ONLY config; returns the
    transcript from after the rc has loaded."""
    with tempfile.NamedTemporaryFile("w", suffix=".hsh", delete=False) as f:
        f.write(rc_body)
        rc = f.name
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_BANNER="0",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1",
               TERM="dumb", PS1="> ")
    pid, fd = pty.fork()
    if pid == 0:
        try:
            os.execve(SHELL, [SHELL, "--rcfile=" + rc, "-i"], env)
        finally:
            os._exit(127)
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
    pump(1.2)
    out = b""
    for c in cmds:
        os.write(fd, c.encode() + b"\n")
        pump(0.7)
    os.write(fd, b"exit\n")
    pump(0.4)
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except OSError:
        pass
    os.unlink(rc)
    return out.decode("utf-8", "replace")


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2

    # 1: precmd fires, once per prompt.
    rc = ("hi() { echo PRECMD_MARK; }\n"
          "HELLISH_PRECMD_FUNCS=(hi)\n")
    out = session(rc, ["true", "true"])
    check("precmd runs before each prompt",
          out.count("PRECMD_MARK") >= 2,
          "saw %d; %r" % (out.count("PRECMD_MARK"), out[-250:]))

    # 2: preexec fires, and is handed the line.
    rc = ("pe() { echo PREEXEC_MARK:$1; }\n"
          "HELLISH_PREEXEC_FUNCS=(pe)\n")
    out = session(rc, ["echo hello world"])
    check("preexec runs with the command line as $1",
          "PREEXEC_MARK:echo hello world" in out, "%r" % out[-300:])

    # 3: preexec runs BEFORE the command, not after.
    check("preexec runs before the command it announces",
          "PREEXEC_MARK" in out
          and out.find("PREEXEC_MARK") < out.rfind("hello world"),
          "ordering wrong; %r" % out[-300:])

    # 4: THE POINT. Two hooks, both run -- the thing trap DEBUG cannot do.
    rc = ("a() { echo HOOK_A; }\nb() { echo HOOK_B; }\n"
          "HELLISH_PRECMD_FUNCS=(a b)\n")
    out = session(rc, ["true"])
    check("two precmd hooks both run, in order",
          "HOOK_A" in out and "HOOK_B" in out
          and out.find("HOOK_A") < out.find("HOOK_B"), "%r" % out[-300:])

    # 5: a plain space-separated string, because that is what someone will
    # write before reading the docs.
    rc = ("a() { echo HOOK_A; }\nb() { echo HOOK_B; }\n"
          "HELLISH_PRECMD_FUNCS='a b'\n")
    out = session(rc, ["true"])
    check("a plain string of names works too",
          "HOOK_A" in out and "HOOK_B" in out, "%r" % out[-300:])

    # 6: $? survives the hook. Without the bracket the prompt reports the
    # hook's status forever, which is the frozen-status bug from #69.
    rc = ("h() { true; }\nHELLISH_PRECMD_FUNCS=(h)\n"
          "PS1='[st=$?] '\n")
    out = session(rc, ["sh -c 'exit 7'"])
    check("a precmd hook does not clobber $?", "[st=7]" in out,
          "the hook's status leaked into the prompt; %r" % out[-300:])

    # 7: non-interactive shells run neither.
    env = dict(os.environ, HELLISH_NO_BANNER="1",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1")
    p = subprocess.run(
        [SHELL, "-c", "h() { echo LEAKED; }; HELLISH_PRECMD_FUNCS=(h)\n"
         "HELLISH_PREEXEC_FUNCS=(h)\necho done"],
        capture_output=True, env=env, timeout=25)
    check("hooks never fire in a non-interactive shell",
          b"LEAKED" not in p.stdout and b"done" in p.stdout,
          "stdout=%r" % p.stdout[:200])

    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

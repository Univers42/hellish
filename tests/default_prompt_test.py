#!/usr/bin/env python3
"""Regression test: the default prompt is plain and lives in a FILE -- #74.

#74 asks for two things:

  * "the prompt by default should be a normal prompt without nothing, just
    maybe the update notification" -- the built-in theme is a two-row box
    with git, timing and job badges, which is a lot to meet on first launch;
  * "this prompt should exist in configuration so we can retouch it and not
    in binary only".

So the seeder writes ~/.config/hellish/rc.d/30-prompt.hsh: three coloured
segments (user, host, cwd) in the shape zsh users already expect, plus \\U for
a pending release. Editing that file is the supported way to change the
prompt -- no rebuild.

Two ordering facts this pins, because getting either wrong makes the feature
silently not happen:

  * rc.d loads BEFORE ~/.hellishrc, so a user's own PS1 still wins. The
    template therefore ships its themes COMMENTED; while it carried an active
    PS1= line it overrode the seeded default every time, and the plain prompt
    was never seen.
  * the seeder must write into the home it was asked to seed. XDG_CONFIG_HOME
    is honoured only when it lives under that home -- otherwise a run with
    HOME overridden (which is how every test drives it) seeds the developer's
    real ~/.config instead. That is not hypothetical; the first version did it.

Usage: python3 default_prompt_test.py [/path/to/hellish]
"""
import os
import pty
import select
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
SEEDER = os.path.join(ROOT, "tools", "seed_hellishrc.sh")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def seed(home, extra_env=None):
    env = dict(os.environ, HOME=home)
    if extra_env:
        env.update(extra_env)
    return subprocess.run(["sh", SEEDER], capture_output=True, text=True,
                          env=env, timeout=60)


def prompt_of(home):
    env = dict(os.environ, HOME=home, HELLISH_NO_BANNER="1",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1", TERM="dumb")
    for v in ("PS1", "PROMPT", "XDG_CONFIG_HOME"):
        env.pop(v, None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL, "-i"], env)
        os._exit(1)
    out = b""
    end = time.time() + 6
    os.write(fd, b"exit\n")
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.3)
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            break
        if not d:
            break
        out += d
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    return out.decode("utf-8", "replace")


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    home = tempfile.mkdtemp()
    pfile = os.path.join(home, ".config", "hellish", "rc.d", "30-prompt.hsh")
    try:
        seed(home)
        check("the seeder writes a prompt file", os.path.isfile(pfile),
              "expected %s" % pfile)

        # It must land in the home we asked for, even though the ambient
        # XDG_CONFIG_HOME points somewhere else entirely.
        real = os.path.expanduser("~/.config/hellish/rc.d/30-prompt.hsh")
        check("seeding a temp home does not touch the real ~/.config",
              not os.path.exists(real)
              or os.path.realpath(real).startswith(os.path.realpath(home)),
              "the seeder wrote to the developer's own config")

        text = prompt_of(home)
        check("the prompt comes from that file, not the built-in theme",
              "\x1b[48;5;24" in text,
              "got %r -- an unstyled or built-in prompt means rc.d lost to "
              "~/.hellishrc" % text[:180])
        # \h is the hostname up to the first dot, as in bash; a 42 machine
        # reports its FQDN (c1r17s4.42madrid.com) and the prompt shows c1r17s4.
        host = os.uname().nodename.split(".")[0]
        for part in (os.environ.get("USER", ""), host):
            if part:
                check("the prompt shows %r" % part, part in text,
                      "got %r" % text[:180])
        check("it is a ONE-line prompt, not the two-row box",
              "╭" not in text and "╰" not in text,
              "the framed built-in theme is still the default")

        # The point of the feature: editing the file changes the prompt.
        with open(pfile, "w") as f:
            f.write("PS1='EDITED> '\n")
        check("editing the file changes the prompt (no rebuild)",
              "EDITED>" in prompt_of(home),
              "got %r" % prompt_of(home)[:150])

        # And it is never clobbered on a re-run.
        seed(home)
        with open(pfile) as f:
            check("a re-run does not overwrite an edited prompt",
                  "EDITED>" in f.read(), "the seeder ate a user's edit")

        # ~/.hellishrc still wins -- rc.d is a default, not a cage.
        with open(os.path.join(home, ".hellishrc"), "a") as f:
            f.write("\nPS1='MINE> '\n")
        check("~/.hellishrc still overrides rc.d",
              "MINE>" in prompt_of(home), "got %r" % prompt_of(home)[:150])
    finally:
        shutil.rmtree(home, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

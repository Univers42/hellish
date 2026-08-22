#!/usr/bin/env python3
"""Regression test: every install route seeds ~/.hellishrc -- issue #51.

The report opens with "I tried to do the command make my_shell ... First
the ~/.hellishrc hasn't been created". It had not, and nothing was going
to create it: the seeding lived inline in user-install.sh, so the sudo
route (`make my_shell` -> install to /usr/bin + chsh) installed a binary
and stopped there. A user who took that route got a shell with no config
file -- no EDITOR, no aliases, no PS1 -- and no hint that one was meant to
exist.

The seeding is now tools/seed_hellishrc.sh, called by both routes, and
this pins the three behaviours that matter:

  * absent   -> seeded from hellishrc.example, byte for byte
  * present  -> NEVER clobbered; the file is the user's, and re-running an
                installer must not eat a config they have edited
  * example missing -> warn and succeed; a missing template is not a
                reason for an install to fail

plus the wiring itself, because a seeder nobody calls is the bug we just
fixed. `make my_shell` needs sudo and chsh and so cannot run in CI, so the
recipe is asserted to invoke the seeder rather than executed.

Usage: python3 hellishrc_seed_test.py [/path/to/hellish]   (shell unused)
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEEDER = os.path.join(ROOT, "tools", "seed_hellishrc.sh")
EXAMPLE = os.path.join(ROOT, "hellishrc.example")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def seed(home, example=EXAMPLE):
    """Run the seeder against a throwaway HOME."""
    env = dict(os.environ)
    env["HOME"] = home
    return subprocess.run(["sh", SEEDER, "--example", example],
                          capture_output=True, text=True, env=env,
                          timeout=30)


def main():
    check("tools/seed_hellishrc.sh exists", os.path.isfile(SEEDER),
          "no seeder at %s" % SEEDER)
    if not os.path.isfile(SEEDER):
        print("\n%d checks failed" % len(FAILS))
        sys.exit(1)
    check("the seeder is a valid shell script",
          subprocess.run(["sh", "-n", SEEDER]).returncode == 0)

    # 1. absent -> seeded, and identical to the template.
    home = tempfile.mkdtemp()
    try:
        p = seed(home)
        rc_path = os.path.join(home, ".hellishrc")
        check("absent: the seeder succeeds", p.returncode == 0,
              "rc=%d %r" % (p.returncode, p.stderr[:200]))
        check("absent: ~/.hellishrc is created", os.path.isfile(rc_path))
        if os.path.isfile(rc_path):
            with open(rc_path) as f:
                got = f.read()
            with open(EXAMPLE) as f:
                want = f.read()
            check("absent: it is hellishrc.example byte for byte",
                  got == want, "%d bytes vs %d" % (len(got), len(want)))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    # 2. present -> untouched. The strongest guarantee here: someone who
    #    has customised their rc must be able to re-run any installer.
    home = tempfile.mkdtemp()
    try:
        rc_path = os.path.join(home, ".hellishrc")
        mine = "# my own rc, do not eat\nalias hi='echo hi'\n"
        with open(rc_path, "w") as f:
            f.write(mine)
        p = seed(home)
        check("present: the seeder still succeeds", p.returncode == 0,
              "rc=%d" % p.returncode)
        with open(rc_path) as f:
            check("present: the existing rc is untouched", f.read() == mine,
                  "the seeder overwrote a user's config")
    finally:
        shutil.rmtree(home, ignore_errors=True)

    # 3. no template -> warn, do not fail, do not create an empty file.
    home = tempfile.mkdtemp()
    try:
        p = seed(home, example=os.path.join(home, "nope.example"))
        check("no template: the seeder does not fail the install",
              p.returncode == 0, "rc=%d" % p.returncode)
        check("no template: no empty ~/.hellishrc is left behind",
              not os.path.exists(os.path.join(home, ".hellishrc")))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    # 4. both install routes actually call it.
    with open(os.path.join(ROOT, "Makefile")) as f:
        mk = f.read()
    target = mk.split("\nmy_shell:", 1)[-1].split("\n\n", 1)[0]
    check("make my_shell calls the seeder", "seed_hellishrc.sh" in target,
          "the sudo route still installs a shell with no config")
    with open(os.path.join(ROOT, "user-install.sh")) as f:
        check("user-install.sh calls the seeder",
              "seed_hellishrc.sh" in f.read(),
              "the two routes have drifted apart again")

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

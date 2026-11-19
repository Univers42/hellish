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


def fake_bin(path, version):
    with open(path, "w") as f:
        f.write('#!/bin/sh\n[ "$1" = --version ] && echo "%s"\nexit 0\n'
                % version)
    os.chmod(path, 0o755)


def tool_sandbox():
    """A PATH with the seeder's tools and NO polish or hellish: the dev box
    running this has its own /usr/bin/hellish, which would decide every
    shebang case below by accident."""
    d = tempfile.mkdtemp(prefix="seed_path_")
    for tool in ("sh", "sed", "awk", "grep", "mkdir", "cp", "mv", "rm", "cat",
                 "head", "basename", "dirname", "date", "chmod", "bash",
                 "printf", "tail", "ls"):
        real = shutil.which(tool)
        if real:
            os.symlink(real, os.path.join(d, tool))
    return d


def shebang_cases():
    """share/ ships `#!/usr/bin/env polish`; the seeder names the command
    this machine really has, so a theme run as a script finds its shell. A
    fresh install never sees the word hellish; an old install the 3.3
    updater upgraded in place has no `polish` command and gets hellish."""
    path = tool_sandbox()
    cases = (("polish", "polish 3.4.0", "polish"),
             ("hellish", "polish 3.4.0", "hellish"),
             ("hellish", "hellish, version 3.3.0", "hellish"))
    for name, version, want in cases:
        home = tempfile.mkdtemp(prefix="seed_shebang_")
        bindir = os.path.join(home, "bin")
        os.makedirs(bindir)
        fake_bin(os.path.join(bindir, name), version)
        env = dict(os.environ, HOME=home, PATH=path)
        env.pop("XDG_CONFIG_HOME", None)
        p = subprocess.run(["sh", SEEDER, "--bin", os.path.join(bindir, name),
                            "--path-dir", bindir], capture_output=True,
                           text=True, env=env, timeout=60)
        # A pre-3.4 binary reads ~/.config/hellish; every 3.4 one, polish.
        brand = "hellish" if version.startswith("hellish") else "polish"
        cfg = os.path.join(home, ".config", brand)
        first = []
        for sub in ("themes/plain.hsh", "rc.d/40-prompt-switch.hsh"):
            f = os.path.join(cfg, sub)
            first.append(open(f).readline() if os.path.isfile(f) else None)
        label = "%s on PATH, %s" % (name, version.split()[0].rstrip(","))
        check("shebang: %s -> %s" % (label, want),
              p.returncode == 0 and first == ["#!/usr/bin/env %s\n" % want] * 2,
              "rc=%d first=%r err=%r" % (p.returncode, first, p.stderr[-200:]))
        if name == "polish":
            hellish_named = [os.path.join(r, f) for r, ds, fs in os.walk(home)
                             for f in fs + ds if "hellish" in f]
            check("shebang: a fresh polish install has no hellish-named file",
                  not hellish_named, repr(hellish_named))
        shutil.rmtree(home, ignore_errors=True)

    # A pre-3.4 --bin must not seed ~/.hellishrc beside a ~/.polishrc.
    home = tempfile.mkdtemp(prefix="seed_shebang_")
    open(os.path.join(home, ".polishrc"), "w").write("# mine\n")
    fake_bin(os.path.join(home, "hellish"), "hellish, version 3.3.0")
    env = dict(os.environ, HOME=home, PATH=path)
    env.pop("XDG_CONFIG_HOME", None)
    p = subprocess.run(["sh", SEEDER, "--bin", os.path.join(home, "hellish")],
                       capture_output=True, text=True, env=env, timeout=60)
    check("old --bin: no ~/.hellishrc seeded beside ~/.polishrc",
          p.returncode == 0
          and not os.path.exists(os.path.join(home, ".hellishrc"))
          and open(os.path.join(home, ".polishrc")).read() == "# mine\n",
          "rc=%d out=%r" % (p.returncode, p.stdout[-200:]))
    shutil.rmtree(home, ignore_errors=True)
    shutil.rmtree(path, ignore_errors=True)


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
        rc_path = os.path.join(home, ".polishrc")
        check("absent: the seeder succeeds", p.returncode == 0,
              "rc=%d %r" % (p.returncode, p.stderr[:200]))
        check("absent: ~/.polishrc is created", os.path.isfile(rc_path))
        check("absent: no legacy ~/.hellishrc is created",
              not os.path.exists(os.path.join(home, ".hellishrc")))
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
        # A ~/.polishrc beside it would win (brand_pick) and silently stop
        # the user's real config from loading after the 3.4.0 rename.
        check("present: no ~/.polishrc shadows the legacy rc",
              not os.path.exists(os.path.join(home, ".polishrc")))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    # 3. no template -> warn, do not fail, do not create an empty file.
    home = tempfile.mkdtemp()
    try:
        p = seed(home, example=os.path.join(home, "nope.example"))
        check("no template: the seeder does not fail the install",
              p.returncode == 0, "rc=%d" % p.returncode)
        check("no template: no empty rc is left behind",
              not os.path.exists(os.path.join(home, ".polishrc"))
              and not os.path.exists(os.path.join(home, ".hellishrc")))
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

    shebang_cases()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

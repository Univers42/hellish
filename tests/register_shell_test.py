#!/usr/bin/env python3
"""Regression test: `make my_shell` cannot half-install or lock you out.

The registration half of `make my_shell` used to be
vendor/scripts/register_shell.sh -- fourteen lines, in a submodule, with no
error handling and no test coverage. In a clean Debian container `make
my_shell` reproducibly died with:

    Registering shell...
    Setting default shell for
    Password: chsh: PAM: Authentication failure
    make: *** [Makefile:591: my_shell] Error 1

...after the binary was already in /usr/bin and /etc/shells was already
edited. This pins the behaviours that made that possible:

  * bare `chsh` (no user operand, no root) prompts for a password, which
    make cannot answer -- registration must escalate and name the user
  * $USER is empty in containers and non-login shells -- the target user
    must come from SUDO_USER/id, never $USER
  * a MISSING /etc/shells made `grep` fail, `!` read that as "absent", and
    the append created a one-entry file -- which makes chsh treat every
    other account on the box as restricted
  * nothing smoke-tested the binary, so chsh would happily install a shell
    that exits 1 immediately -- a real lockout, fixable only as root
  * every failure landed AFTER a multi-minute rebuild

`make my_shell` itself needs root and chsh, so like hellishrc_seed_test.py
the wiring is asserted rather than executed; the script's own logic is
exercised through --dry-run and --preflight, which touch nothing.

Usage: python3 register_shell_test.py [/path/to/hellish]   (shell unused)
"""
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REG = os.path.join(ROOT, "tools", "register_shell.sh")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(args, env=None, **kw):
    e = dict(os.environ)
    e.setdefault("HOME", "/tmp")
    if env:
        e.update(env)
    return subprocess.run(["sh", REG] + args, capture_output=True, text=True,
                          env=e, timeout=60, **kw)


def fake_shell(path, script):
    with open(path, "w") as f:
        f.write(script)
    os.chmod(path, 0o755)
    return path


def main():
    check("tools/register_shell.sh exists", os.path.isfile(REG),
          "no registrar at %s" % REG)
    if not os.path.isfile(REG):
        print("\n%d checks failed" % len(FAILS))
        sys.exit(1)
    check("it is a valid POSIX shell script",
          subprocess.run(["sh", "-n", REG]).returncode == 0)
    check("it is executable",
          bool(os.stat(REG).st_mode & stat.S_IXUSR))

    src = open(REG).read()

    # 1. chsh must be escalated AND told which user. A bare `chsh -s X` is
    #    the exact line that prompted for a password under make.
    #    Only real INVOCATIONS count: strip double-quoted strings first, so
    #    the `sudo chsh -s ...` inside an error message is not mistaken for
    #    one, and require -s, so `command -v chsh` is not either.
    chsh_calls = [ln for ln in src.splitlines()
                  if not ln.lstrip().startswith("#")
                  and re.search(r"\bchsh\s+-s", re.sub(r'"[^"]*"', '""', ln))]
    check("the script calls chsh at all", bool(chsh_calls))
    check("every chsh call is escalated to root",
          all("as_root" in c for c in chsh_calls),
          "un-escalated: %r" % [c.strip() for c in chsh_calls
                                if "as_root" not in c])
    check("every chsh call names the target user explicitly",
          all(("TARGET_USER" in c or "$1" in c) for c in chsh_calls),
          "a chsh with no user operand retargets the real uid: %r"
          % [c.strip() for c in chsh_calls if "TARGET_USER" not in c])

    # 2. $USER is empty in a container; SUDO_USER/id -un are not.
    check("the target user is not taken from $USER",
          not re.search(r'TARGET_USER=.*\$\{?USER\b', src),
          "$USER is unset in containers and non-login shells")
    check("the target user falls back to SUDO_USER then id -un",
          "SUDO_USER" in src and "id -un" in src)

    # 3. the /etc/shells clobber: grep on a missing file must not read as
    #    "not listed yet" and produce a one-entry whitelist.
    check("a missing /etc/shells is guarded before grep",
          re.search(r"\[ -f /etc/shells \]", src) is not None,
          "grep failing on a missing file reads as 'absent' and clobbers it")
    check("a created /etc/shells keeps the shells already in use",
          "seed_etc_shells" in src and "getent passwd" in src,
          "a one-entry /etc/shells makes chsh refuse every other account")
    check("...but never seeds nologin/false/sync into it",
          re.search(r"grep -vE '/\(nologin\|false\|true\|sync\)\$'", src)
          is not None,
          "/etc/shells means 'valid login shell'; locked accounts' shells "
          "must not be listed there")

    # 4. the lockout guard: prove the binary runs BEFORE chsh touches passwd.
    check("the binary is smoke-tested", "smoke_test" in src)
    smoke_at = src.find("smoke_test \"$BIN_SRC\"")
    chsh_at = src.find("as_root chsh")
    check("the smoke test runs BEFORE chsh",
          -1 < smoke_at < chsh_at,
          "smoke=%d chsh=%d" % (smoke_at, chsh_at))

    # 5. --preflight must change NOTHING and must fail loudly when the
    #    machine cannot do it. Run it for real: it is side-effect free.
    p = run(["--preflight"])
    check("--preflight runs without touching anything",
          p.returncode in (0, 1),
          "rc=%d %r" % (p.returncode, p.stderr[-200:]))

    # ...and with no chsh on PATH it must FAIL, not sail on to the build.
    # A PATH sandbox, not an empty PATH: emptying it would hide `sh` itself
    # and prove nothing. Every tool the script uses is linked in EXCEPT chsh.
    nochsh = tempfile.mkdtemp()
    for tool in ("sh", "id", "getent", "grep", "awk", "sort", "mktemp",
                 "tee", "install", "sed", "cat", "sudo", "rm", "basename",
                 "printf"):
        real = shutil.which(tool)
        if real:
            os.symlink(real, os.path.join(nochsh, tool))
    check("the PATH sandbox really has no chsh",
          shutil.which("chsh", path=nochsh) is None)
    p = run(["--preflight"], env={"PATH": nochsh})
    check("--preflight fails when chsh is missing", p.returncode != 0,
          "rc=%d — a machine with no chsh would rebuild for minutes first"
          % p.returncode)
    check("--preflight says what to do instead",
          "user-install" in (p.stderr + p.stdout),
          "the no-root route is the answer and must be named")

    # 6. a binary that does not run must never reach chsh.
    tmp = tempfile.mkdtemp()
    broken = fake_shell(os.path.join(tmp, "broken"), "#!/bin/sh\nexit 1\n")
    p = run(["--dry-run", "--bin", broken, "--dest",
             os.path.join(tmp, "dest")])
    check("a broken binary is refused", p.returncode != 0,
          "rc=%d — chsh would have made it your login shell" % p.returncode)
    check("...and the refusal says why",
          "smoke test failed" in (p.stderr + p.stdout),
          p.stderr[-200:])
    check("...and chsh was never reached",
          "chsh" not in p.stderr.split("smoke test failed")[0],
          "chsh ran before the binary was proven")

    # 7. a missing binary is a clear message, not a stack of shell errors.
    p = run(["--dry-run", "--bin", os.path.join(tmp, "nope")])
    check("a missing binary is reported clearly", p.returncode != 0
          and "no binary at" in (p.stderr + p.stdout), p.stderr[-200:])

    # 8. the wiring: my_shell must use THIS script, preflight FIRST, and must
    #    no longer reach into the submodule.
    with open(os.path.join(ROOT, "Makefile")) as f:
        mk = f.read()
    target = mk.split("\nmy_shell:", 1)[-1].split("\n\n", 1)[0]
    check("make my_shell calls tools/register_shell.sh",
          "tools/register_shell.sh" in target)
    check("make my_shell no longer calls the submodule script",
          "vendor/scripts/register_shell.sh" not in target,
          "the unchecked 14-line version is back in the path")
    check("make my_shell still seeds the config", "seed_hellishrc.sh" in target)
    pre = target.find("--preflight")
    build = max(target.find("static-verify"), target.find("re OPT=1"))
    check("--preflight runs BEFORE the rebuild", -1 < pre < build,
          "preflight=%d build=%d" % (pre, build))
    check("my_shell no longer runs a bare `sudo install`",
          not re.search(r"^\s*sudo install", target, re.M),
          "the install must go through the smoke-tested path")

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

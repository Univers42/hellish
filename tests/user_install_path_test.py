#!/usr/bin/env python3
"""Regression test: `make user-install` leaves `hellish` on PATH.

THE BUG. user-install.sh puts the binary in $PREFIX/bin (default
~/.local/bin) and appends an `exec` hook to your login shell's rc. Both
halves worked, and the shell came up -- so the gap was invisible from the
outside. What was missing was the NAME: nothing on either route ever put
$PREFIX/bin on PATH, so on a machine where the login chain had not already
done it, a freshly installed user got

    $ hellish update
    hellish: command not found

inside hellish and out. `command -v hellish`, `hellish --version`, and
every tool that looks a shell up by name answered the same way.

WHY THE LOGIN CHAIN DOES NOT COVER IT. ~/.hellishrc's own comments used to
argue that a PATH line here was redundant, because a LOGIN hellish sources
/etc/profile and then ~/.profile, which is what puts ~/.local/bin on PATH.
That is true of the `make my_shell` + chsh route and false of this one,
twice over:

  * user-install's hellish is exec'd from an INTERACTIVE rc. It is not a
    login shell, so it reads neither file.
  * Debian/Ubuntu's ~/.profile adds ~/.local/bin only `if [ -d ]` -- and on
    a first install, this install is what creates that directory. Even a
    login shell would not have picked it up until the next logout.

THE FIX. tools/seed_hellishrc.sh (the seeder both install routes share)
maintains one marker-delimited block in ~/.hellishrc that puts the real
install directory on PATH, and user-install.sh's rc hook does the same
before it execs. Both are guarded against re-sourcing, both name the
directory actually installed into, and both are removed by
`make user-uninstall`.

Hermetic: every case runs against a temporary HOME and a temporary rc file,
passes --bin so nothing is compiled, and never touches the real $HOME.

Usage: python3 user_install_path_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INSTALLER = os.path.join(ROOT, "user-install.sh")
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))

# A PATH that deliberately does NOT contain any install prefix we use. This
# is the whole point: it stands in for the machine where the login chain
# never added ~/.local/bin.
CLEAN_PATH = "/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

BASE_ENV = {
    "PATH": CLEAN_PATH,
    "TERM": "xterm-256color",
    "LANG": "C.UTF-8",
    "HELLISH_NO_BANNER": "1",
    "HELLISH_NO_UPDATE_CHECK": "1",
    "HELLISH_NO_ANIM": "1",
    "ASAN_OPTIONS": "detect_leaks=0",
}

FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


# ── the installer, driven the way `make user-install` drives it ─────────────
def install(home, prefix=None, rc=None, uninstall=False):
    """Run user-install.sh against a temporary HOME. Returns CompletedProcess."""
    prefix = prefix or os.path.join(home, ".local")
    rc = rc or os.path.join(home, ".bashrc")
    env = dict(BASE_ENV)
    env["HOME"] = home
    argv = [INSTALLER, "--prefix", prefix, "--rc", rc]
    if uninstall:
        argv.append("--uninstall")
    else:
        argv += ["--bin", SHELL]
    return subprocess.run(argv, cwd=ROOT, env=env, capture_output=True,
                          text=True, timeout=180)


def new_home(rc_lines="# a login rc that was here first\nexport FOO=bar\n"):
    home = tempfile.mkdtemp(prefix="hellish-userinstall-")
    with open(os.path.join(home, ".bashrc"), "w") as f:
        f.write(rc_lines)
    return home


def run_in_shell(home, bindir, script):
    """Run `script` in the INSTALLED hellish, with a PATH lacking bindir."""
    env = dict(BASE_ENV)
    env["HOME"] = home
    return subprocess.run([os.path.join(bindir, "hellish"), "-c", script],
                          env=env, capture_output=True, text=True, timeout=60)


def block_count(path, marker):
    if not os.path.exists(path):
        return 0
    with open(path) as f:
        return sum(1 for line in f if line.rstrip("\n") == marker)


RC_MARK = "# >>> hellish path >>>"
HOOK_MARK = "# >>> hellish >>>"


# ── 1. the reported bug, in the shape the user meets it ────────────────────
def test_name_resolves_after_install():
    home = new_home()
    try:
        bindir = os.path.join(home, ".local", "bin")
        r = install(home)
        check("installer succeeds", r.returncode == 0, r.stderr[-400:])

        rc = os.path.join(home, ".hellishrc")
        check("~/.hellishrc is seeded", os.path.isfile(rc))

        # The bug: sourcing ~/.hellishrc left PATH untouched, so the shell
        # could not find itself by name.
        got = run_in_shell(home, bindir,
                           '. "$HOME/.hellishrc" >/dev/null 2>&1; '
                           'command -v hellish')
        want = os.path.join(bindir, "hellish")
        check("~/.hellishrc puts the install dir on PATH",
              got.stdout.strip() == want,
              "got %r, want %r" % (got.stdout.strip(), want))

        # ... and the same thing said the way a user says it.
        got = run_in_shell(home, bindir,
                           '. "$HOME/.hellishrc" >/dev/null 2>&1; '
                           'hellish -c "echo REACHED-BY-NAME"')
        check("`hellish` runs as a bare command name",
              "REACHED-BY-NAME" in got.stdout,
              "stdout=%r stderr=%r" % (got.stdout, got.stderr[-200:]))
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 2. the same thing, end to end, in a real terminal ──────────────────────
def test_interactive_shell_finds_itself():
    """An interactive hellish sources ~/.hellishrc; that is the real path
    a user's session takes, and the only one that proves the rc is read at
    startup rather than merely being sourceable."""
    home = new_home()
    try:
        bindir = os.path.join(home, ".local", "bin")
        install(home)
        env = dict(BASE_ENV)
        env["HOME"] = home
        env["PS1"] = "$ "

        pid, fd = pty.fork()
        if pid == 0:
            os.environ.clear()
            os.environ.update(env)
            os.execv(os.path.join(bindir, "hellish"),
                     [os.path.join(bindir, "hellish")])
        time.sleep(0.8)
        os.write(fd, b'command -v hellish\n')
        time.sleep(0.5)
        os.write(fd, b'exit\n')

        out = b""
        deadline = time.time() + 12
        while time.time() < deadline:
            r, _, _ = select.select([fd], [], [], 0.4)
            if not r:
                continue
            try:
                chunk = os.read(fd, 65536)
            except OSError:
                break
            if not chunk:
                break
            out += chunk
        os.close(fd)
        try:
            os.waitpid(pid, 0)
        except ChildProcessError:
            pass

        text = re.sub(r"\x1b\[[0-9;?]*[A-Za-z]", "",
                      out.decode("utf-8", "replace"))
        want = os.path.join(bindir, "hellish")
        check("an interactive hellish resolves its own name", want in text,
              "pty output tail: %r" % text[-300:])
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 3. the guard: an rc is re-sourced, PATH must not grow ──────────────────
def test_no_duplicate_path_entries():
    home = new_home()
    try:
        bindir = os.path.join(home, ".local", "bin")
        install(home)
        got = run_in_shell(
            home, bindir,
            '. "$HOME/.hellishrc" >/dev/null 2>&1; '
            '. "$HOME/.hellishrc" >/dev/null 2>&1; '
            '. "$HOME/.hellishrc" >/dev/null 2>&1; echo "$PATH"')
        n = got.stdout.strip().split(":").count(bindir)
        check("three sources leave exactly one PATH entry", n == 1,
              "counted %d" % n)
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 4. the login shell's rc hook carries PATH across the exec too ──────────
def test_login_rc_hook_exports_path():
    home = new_home()
    try:
        bindir = os.path.join(home, ".local", "bin")
        rc = os.path.join(home, ".bashrc")
        install(home)
        env = dict(BASE_ENV)
        env["HOME"] = home
        # HELLISH_NO_EXEC=1 stops the handover so we can look at what the
        # hook did to the environment BEFORE it would have exec'd.
        env["HELLISH_NO_EXEC"] = "1"
        got = subprocess.run(
            ["/bin/sh", "-c", '. "$HOME/.bashrc"; command -v hellish'],
            env=env, capture_output=True, text=True, timeout=60)
        check("the login-rc hook puts the dir on PATH as well",
              got.stdout.strip() == os.path.join(bindir, "hellish"),
              "got %r" % got.stdout.strip())
        check("the hook still parses as POSIX sh",
              subprocess.run(["/bin/sh", "-n", rc]).returncode == 0)
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 5. it must name the directory it REALLY installed into ────────────────
def test_custom_prefix_is_honoured():
    home = new_home()
    try:
        prefix = os.path.join(home, "opt", "hellish")
        bindir = os.path.join(prefix, "bin")
        install(home, prefix=prefix)
        got = run_in_shell(home, bindir,
                           '. "$HOME/.hellishrc" >/dev/null 2>&1; '
                           'command -v hellish')
        check("a custom --prefix is what lands in the block",
              got.stdout.strip() == os.path.join(bindir, "hellish"),
              "got %r" % got.stdout.strip())
        with open(os.path.join(home, ".hellishrc")) as f:
            body = f.read()
        check("no hardcoded ~/.local/bin leaks into the block",
              os.path.join(home, ".local", "bin") not in body)
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 6. the rule that must survive all of this: your rc is YOURS ───────────
def test_existing_rc_is_not_clobbered():
    home = new_home()
    try:
        rc = os.path.join(home, ".hellishrc")
        mine = ("# hand-written\nalias mine='echo SENTINEL-KEPT'\n"
                "export MY_OWN_VAR=42\n")
        with open(rc, "w") as f:
            f.write(mine)
        bindir = os.path.join(home, ".local", "bin")
        install(home)
        with open(rc) as f:
            body = f.read()
        check("every hand-written line survives",
              all(line in body for line in mine.strip().splitlines()))
        check("...and the PATH block was still added",
              block_count(rc, RC_MARK) == 1)
        got = run_in_shell(home, bindir,
                           '. "$HOME/.hellishrc" >/dev/null 2>&1; '
                           'echo "$MY_OWN_VAR"; command -v hellish')
        check("the merged rc still works",
              got.stdout.split() == ["42", os.path.join(bindir, "hellish")],
              "got %r" % got.stdout)
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 7. re-running an installer is normal; stacking blocks is not ──────────
def test_reinstall_is_idempotent():
    home = new_home()
    try:
        rc = os.path.join(home, ".hellishrc")
        hook = os.path.join(home, ".bashrc")
        for _ in range(3):
            install(home)
        check("~/.hellishrc holds exactly one PATH block",
              block_count(rc, RC_MARK) == 1,
              "found %d" % block_count(rc, RC_MARK))
        check("the login rc holds exactly one hook block",
              block_count(hook, HOOK_MARK) == 1,
              "found %d" % block_count(hook, HOOK_MARK))
        bindir = os.path.join(home, ".local", "bin")
        got = run_in_shell(home, bindir,
                           '. "$HOME/.hellishrc" >/dev/null 2>&1; echo "$PATH"')
        check("PATH still holds one copy after three installs",
              got.stdout.strip().split(":").count(bindir) == 1)
    finally:
        shutil.rmtree(home, ignore_errors=True)


# ── 8. and it all comes back out again ────────────────────────────────────
def test_uninstall_removes_only_our_block():
    home = new_home()
    try:
        rc = os.path.join(home, ".hellishrc")
        install(home)
        with open(rc, "a") as f:
            f.write("\nalias after='echo AFTER-THE-BLOCK'\n")
        r = install(home, uninstall=True)
        check("uninstall succeeds", r.returncode == 0, r.stderr[-300:])
        with open(rc) as f:
            body = f.read()
        check("the PATH block is gone", block_count(rc, RC_MARK) == 0)
        check("the user's own lines are not",
              "AFTER-THE-BLOCK" in body and "alias ll=" in body)
        check("~/.hellishrc still parses",
              subprocess.run(["/bin/sh", "-n", rc]).returncode == 0)
        check("the hook block is gone too",
              block_count(os.path.join(home, ".bashrc"), HOOK_MARK) == 0)
    finally:
        shutil.rmtree(home, ignore_errors=True)


def main():
    if not os.access(SHELL, os.X_OK):
        print("FAIL no shell at %s -- run make" % SHELL)
        return 2
    print("shell:     %s" % SHELL)
    print("installer: %s\n" % INSTALLER)
    for fn in (test_name_resolves_after_install,
               test_interactive_shell_finds_itself,
               test_no_duplicate_path_entries,
               test_login_rc_hook_exports_path,
               test_custom_prefix_is_honoured,
               test_existing_rc_is_not_clobbered,
               test_reinstall_is_idempotent,
               test_uninstall_removes_only_our_block):
        print("── %s" % fn.__name__)
        fn()
    print("\n%d checks failed" % len(FAILS))
    for f in FAILS:
        print("  FAIL %s" % f)
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

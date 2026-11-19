#!/usr/bin/env python3
"""3.4.0 rename (hellish -> polish): an upgrade must not break an old install.

A user who installed hellish <= 3.3.0 and runs `update` gets the 3.4.0 binary
over the same path, with nothing else on disk changed. Everything they wrote
for the old name has to keep working, and a NEW user must only ever see the
new one. Each case below is one thing such a user has on disk:

  ~/.hellishrc alone            -> still loaded (a lost config is the worst
                                   outcome of an update)
  ~/.polishrc and ~/.hellishrc  -> the new one wins, the old is NOT also run
  $XDG_CONFIG_HOME/hellish/     -> its rc.d still loads; polish/ wins if both
  HELLISH_PRECMD_FUNCS in an rc -> still runs; POLISH_ wins if both are set,
                                   and the hook runs ONCE, not once per prefix
  $HELLISH_GIT_* in a PS1       -> still published next to $POLISH_GIT_*
  foo.hellish (not executable)  -> still interpreted by us, like foo.polish

and the one contract that is frozen in binaries already in the field: the
<= 3.3.0 updater validates a downloaded binary by running
`<new> -c "update --version"` and requiring everything after the first space
to equal the release tag minus its `v` (v3.3.0 update_apply.c, validate_binary
+ parse_tag). If that ever fails, no old install can update past it again --
so it is replayed here with the old rule, not with a regex of the new output.

Not covered: /etc/polish vs /etc/hellish (needs root; same brand_pick as the
XDG case, which is).

Usage: python3 brand_compat_test.py [/path/to/shell]
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
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "polish"))
QUIET = {"HELLISH_NO_BANNER": "1", "HELLISH_BANNER": "0",
         "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
         "TERM": "dumb", "PS1": "> "}
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def base_env(home):
    env = {k: v for k, v in os.environ.items()
           if not k.startswith(("HELLISH_", "POLISH_", "XDG_CONFIG_HOME"))}
    env.update(QUIET, HOME=home)
    return env


def interactive(home, cmds=(), extra=None):
    """A real tty session (rc files load only on readline input); returns
    the whole transcript, rc output included."""
    env = base_env(home)
    env.update(extra or {})
    pid, fd = pty.fork()
    if pid == 0:
        try:
            os.chdir(home)
            os.execve(SHELL, [SHELL, "-i"], env)
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
    for c in cmds:
        os.write(fd, c.encode() + b"\n")
        pump(0.6)
    os.write(fd, b"exit\n")
    pump(0.4)
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    return out.decode("utf-8", "replace")


def run_c(home, script, extra=None, cwd=None):
    env = base_env(home)
    env.update(extra or {})
    return subprocess.run([SHELL, "-c", script], env=env, cwd=cwd or home,
                          capture_output=True, text=True, timeout=20)


def write(path, body):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(body)


def fresh():
    return tempfile.mkdtemp(prefix="brand_compat_")


def test_rc_files():
    h = fresh()
    write(h + "/.hellishrc", "echo RC-LEGACY-LOADED\n")
    out = interactive(h)
    check("only ~/.hellishrc: loaded", "RC-LEGACY-LOADED" in out, out[-300:])
    write(h + "/.polishrc", "echo RC-NEW-LOADED\n")
    out = interactive(h)
    check("both rc files: ~/.polishrc loaded", "RC-NEW-LOADED" in out,
          out[-300:])
    check("both rc files: ~/.hellishrc NOT also run",
          "RC-LEGACY-LOADED" not in out, out[-300:])
    shutil.rmtree(h)
    h = fresh()
    write(h + "/.polishrc", "echo RC-NEW-LOADED\n")
    out = interactive(h)
    check("only ~/.polishrc: loaded", "RC-NEW-LOADED" in out, out[-300:])
    shutil.rmtree(h)


def test_xdg_dirs():
    h = fresh()
    xdg = {"XDG_CONFIG_HOME": h + "/xdg"}
    write(h + "/xdg/hellish/rc.d/10-a.hsh", "echo XDG-LEGACY-LOADED\n")
    out = interactive(h, extra=xdg)
    check("only $XDG/hellish: rc.d loaded", "XDG-LEGACY-LOADED" in out,
          out[-300:])
    write(h + "/xdg/polish/rc.d/10-a.hsh", "echo XDG-NEW-LOADED\n")
    out = interactive(h, extra=xdg)
    check("both XDG dirs: polish/ wins", "XDG-NEW-LOADED" in out
          and "XDG-LEGACY-LOADED" not in out, out[-300:])
    shutil.rmtree(h)


def hook_rc(decls):
    return ("old() { echo HOOK-OLD; }\nnew() { echo HOOK-NEW; }\n"
            + decls + "\n")


def test_hook_prefixes():
    h = fresh()
    write(h + "/.hellishrc", hook_rc("HELLISH_PRECMD_FUNCS=(old)"))
    out = interactive(h, ["true"])
    check("HELLISH_PRECMD_FUNCS alone still runs", "HOOK-OLD" in out,
          out[-300:])
    write(h + "/.hellishrc", hook_rc("POLISH_PRECMD_FUNCS=(new)\n"
                                     "HELLISH_PRECMD_FUNCS=(old)"))
    out = interactive(h, ["true"])
    check("both prefixes: POLISH_ wins, HELLISH_ not also run",
          "HOOK-NEW" in out and "HOOK-OLD" not in out, out[-300:])
    shutil.rmtree(h)


def test_git_vars():
    h = fresh()
    repo = h + "/repo"
    os.makedirs(repo)
    git = ["git", "-C", repo, "-c", "user.name=t", "-c", "user.email=t@t"]
    subprocess.run(git + ["init", "-q", "-b", "brandcheck"], check=True)
    subprocess.run(git + ["commit", "-q", "--allow-empty", "-m", "x"],
                   check=True)
    r = run_c(h, 'vcs_info; echo "$POLISH_GIT_BRANCH|$HELLISH_GIT_BRANCH"',
              cwd=repo)
    check("GIT_BRANCH published under both prefixes",
          r.stdout.strip() == "brandcheck|brandcheck",
          repr(r.stdout) + repr(r.stderr[-200:]))
    shutil.rmtree(h)


def test_script_extensions():
    """An executable script with no shebang runs under US when its extension
    is ours, else under /bin/sh (run_utils.c, is_own_script). `update` is a
    builtin only we have, so it tells the two interpreters apart."""
    h = fresh()
    for ext in ("polish", "hellish", "hell"):
        write(h + "/s." + ext, "update --version\n")
        os.chmod(h + "/s." + ext, 0o755)
        r = run_c(h, "./s." + ext)
        check("shebang-less ." + ext + " runs under this shell",
              r.returncode == 0 and r.stdout.startswith("polish "),
              "rc=%d %r" % (r.returncode, r.stdout))
    write(h + "/s.txt", "update --version\n")
    os.chmod(h + "/s.txt", 0o755)
    r = run_c(h, "./s.txt")
    check("shebang-less .txt still goes to /bin/sh",
          not r.stdout.startswith("polish "), repr(r.stdout))
    shutil.rmtree(h)


def released_version():
    with open(os.path.join(ROOT, "incs", "version.h")) as f:
        return re.search(r'HELLISH_VERSION\s+"([^"]+)"', f.read()).group(1)


def test_old_updater_contract():
    """validate_binary as shipped in v3.3.0: first line of stdout, the text
    after the FIRST space, compared byte-for-byte to the tag minus `v`."""
    h = fresh()
    tag = "v" + released_version()
    r = run_c(h, "update --version")
    line = r.stdout.split("\n", 1)[0]
    got = line.split(" ", 1)[1] if " " in line else ""
    check("old validate_binary accepts `update --version`",
          r.returncode == 0 and got == tag[1:],
          "stdout=%r want rest=%r" % (r.stdout, tag[1:]))
    check("`update --version` prints the new name", line.startswith("polish "),
          repr(line))
    check("`update --version` prints nothing else", r.stdout == line + "\n",
          repr(r.stdout))
    shutil.rmtree(h)


def main():
    if not os.access(SHELL, os.X_OK):
        print("no shell at " + SHELL)
        return 1
    test_old_updater_contract()
    test_script_extensions()
    test_git_vars()
    test_rc_files()
    test_xdg_dirs()
    test_hook_prefixes()
    print("%d failure(s)" % len(FAILS) if FAILS else "all brand checks pass")
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""vcs_info's format language and the zstyle keys behind it, diffed against
a real zsh -- the half of issue #112 that is not a parse error.

The reporter's rc configures vcs_info the way its manual says to:

    zstyle ':vcs_info:git:*' enable git
    zstyle ':vcs_info:git:*' check-for-changes true
    zstyle ':vcs_info:git:*' stagedstr '%F{green}+%f'
    zstyle ':vcs_info:git:*' unstagedstr '%F{red}*%f'
    zstyle ':vcs_info:git:*' formats '%F{242}on%f %F{magenta} %b%c%u%f '

and every line but `formats` was answered with "zstyle: not supported
(needs the zsh completion system)" -- at every shell start -- while the
format itself lost its colours: an unknown %x was consumed EMPTY, so
`%F{242}` came out as `{242}`. zsh's vcs_info touches only its own
specs and leaves prompt escapes for the prompt to expand.

Pinned here against zsh 5.9 (or whatever zsh is on the box; skipped
cleanly with none), state by state in a scratch repository:

    %b branch   %c stagedstr if the index differs from HEAD
    %s "git"    %u unstagedstr if the work tree differs from the index
    %r repo     %% a percent   anything else: untouched

Both markers exist only under `check-for-changes true`, exactly like zsh,
and untracked files never count (zsh's default, and our -uno scan).

Usage: python3 vcs_info_zstyle_test.py [/path/to/hellish]
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

FMT = "%b|%c|%u|%s|%r|%%|%F{red}x%f"
STYLES = ("zstyle ':vcs_info:git:*' enable git\n"
          "zstyle ':vcs_info:git:*' check-for-changes true\n"
          "zstyle ':vcs_info:git:*' stagedstr '+'\n"
          "zstyle ':vcs_info:git:*' unstagedstr '*'\n")
SCRIPT = ("autoload -Uz vcs_info\n%s"
          "zstyle ':vcs_info:git:*' formats '%s'\n"
          "vcs_info\nprint -r -- \"[$vcs_info_msg_0_]\"\n")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def find_zsh():
    env = os.environ.get("ZSH_ORACLE")
    if env and os.path.exists(env):
        return env
    home = os.path.expanduser("~/zsh-5.9/bin/zsh")
    for c in (home, shutil.which("zsh"), "/usr/bin/zsh", "/bin/zsh"):
        if c and os.path.exists(c):
            return c
    return None


def run(argv, cwd):
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")
    try:
        p = subprocess.run(argv, cwd=cwd, env=env, capture_output=True,
                           timeout=20)
        return p.returncode, p.stdout.decode("utf-8", "replace"), \
            p.stderr.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return -1, "", "<timeout>"


def git(repo, *args):
    subprocess.run(["git", "-c", "user.email=t@t", "-c", "user.name=t"]
                   + list(args), cwd=repo, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def both(zsh, repo, script):
    path = os.path.join(repo, "case.zsh")
    with open(path, "w") as f:
        f.write(script)
    _, zout, _ = run([zsh, "-f", path], repo)
    _, hout, herr = run([SHELL, "-c", "source ./case.zsh"], repo)
    return zout, hout, herr


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    zsh = find_zsh()
    if not zsh or not shutil.which("git"):
        print("skip: no zsh or no git on this machine")
        return 0
    top = tempfile.mkdtemp()
    repo = os.path.join(top, "vrepo")
    os.makedirs(repo)
    git(repo, "init", "-q", "-b", "main")
    git(repo, "commit", "-q", "--allow-empty", "-m", "init")
    with open(os.path.join(repo, "a.txt"), "w") as f:
        f.write("a\n")
    git(repo, "add", "a.txt")
    git(repo, "commit", "-q", "-m", "a")

    def state(name, prepare):
        prepare()
        zout, hout, herr = both(zsh, repo, SCRIPT % (STYLES, FMT))
        check("vcs_info/%s" % name, zout == hout and zout != "",
              "zsh=%r hellish=%r %r" % (zout, hout, herr[:120]))
        check("vcs_info/%s: no zstyle complaint on stderr" % name,
              herr == "", herr[:200])

    def append(text):
        with open(os.path.join(repo, "a.txt"), "a") as f:
            f.write(text)

    state("clean", lambda: None)
    state("unstaged", lambda: append("b\n"))
    state("staged", lambda: git(repo, "add", "a.txt"))
    state("staged+unstaged", lambda: append("c\n"))
    state("untracked-does-not-count",
          lambda: open(os.path.join(repo, "new.txt"), "w").close())
    # Without check-for-changes, zsh leaves %c and %u empty even on a
    # dirty tree -- the markers are opt-in because the scan costs a fork.
    zout, hout, herr = both(zsh, repo, SCRIPT % (
        "zstyle ':vcs_info:git:*' stagedstr '+'\n"
        "zstyle ':vcs_info:git:*' unstagedstr '*'\n", "%b|%c|%u"))
    check("vcs_info/markers need check-for-changes", zout == hout
          and zout == "[main||]\n", "zsh=%r hellish=%r" % (zout, hout))
    # Outside a repository the message is empty, as in zsh.
    zout, hout, herr = both(zsh, top, SCRIPT % (STYLES, FMT))
    check("vcs_info/outside a repo the message is empty",
          zout == hout == "[]\n", "zsh=%r hellish=%r" % (zout, hout))
    # A style hellish cannot honour outside vcs_info is still said, once:
    # the completion system is a real gap and silence would hide it.
    rc, out, err = run([SHELL, "-c",
                        "zstyle ':completion:*' menu select; echo st=$?"],
                       top)
    check("zstyle/completion styles still report the gap",
          "not supported" in err and "st=1" in out,
          "out=%r err=%r" % (out, err[:120]))
    shutil.rmtree(top, ignore_errors=True)
    print("\n%s" % ("ALL PASSED" if not FAILS else "%d FAILED: %s"
                    % (len(FAILS), ", ".join(FAILS))))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

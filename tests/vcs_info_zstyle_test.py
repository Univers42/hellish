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

The HELLISH_GIT_* variables vcs_info also sets -- the fork-free source a
prompt framework reads instead of running git seven times per prompt --
have no zsh counterpart, so they are checked against git's own answers,
with or without zsh on the machine: ahead/behind, stash, each kind of
change, a conflicted merge, a detached HEAD, a submodule with local edits
(ignored, as zsh's vcs_info does), a subdirectory of a submodule (whose
relative gitdir used to be resolved against the cwd) and a linked
worktree.

Usage: python3 vcs_info_zstyle_test.py [/path/to/hellish]
"""
import os
import re
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


def git_version():
    """(major, minor) of the git on PATH -- the one the shell will run."""
    out = subprocess.run(["git", "--version"], capture_output=True).stdout
    m = re.search(rb"(\d+)\.(\d+)", out)
    if not m:
        return (0, 0)
    return (int(m.group(1)), int(m.group(2)))


def both(zsh, repo, script):
    path = os.path.join(repo, "case.zsh")
    with open(path, "w") as f:
        f.write(script)
    _, zout, _ = run([zsh, "-f", path], repo)
    _, hout, herr = run([SHELL, "-c", "source ./case.zsh"], repo)
    return zout, hout, herr


GITVARS = ("BRANCH", "DETACHED", "ROOT", "DIR", "STAGED", "UNSTAGED",
           "UNTRACKED", "UNMERGED", "AHEAD", "BEHIND", "STASH")


def git_vars(cwd, untracked=False):
    """HELLISH_GIT_* as vcs_info leaves them in `cwd`. The scan is
    asynchronous: the second call harvests whatever the first one started,
    once the sleep has given it time to finish."""
    pre = "HELLISH_VCS_UNTRACKED=1; " if untracked else ""
    probe = "; ".join('printf "%s=%%s\\n" "$HELLISH_GIT_%s"' % (v, v)
                      for v in GITVARS)
    rc, out, err = run([SHELL, "--norc", "-c", pre + "vcs_info; sleep 1; "
                        "vcs_info; " + probe], cwd)
    got = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return got, err


def expect(name, cwd, want, untracked=False):
    # The scan is asynchronous and a loaded machine can take longer than
    # the sleep in git_vars; a few more looks are allowed before a
    # mismatch counts. A wrong answer stays wrong however long it runs.
    for _ in range(5):
        got, err = git_vars(cwd, untracked)
        bad = {k: (got.get(k), v) for k, v in want.items()
               if got.get(k) != v}
        if not bad:
            break
    check("HELLISH_GIT/%s" % name, not bad and not err,
          "got/want %r %r" % (bad, err[:120]))
    return got


def commit(repo, name, text):
    with open(os.path.join(repo, name), "w") as f:
        f.write(text)
    git(repo, "add", name)
    git(repo, "commit", "-q", "-m", name)


def git_vars_cases(top):
    up = os.path.join(top, "up.git")
    a = os.path.join(top, "a")
    b = os.path.join(top, "b")
    git(top, "init", "-q", "--bare", "-b", "main", up)
    git(top, "clone", "-q", up, a)
    commit(a, "f", "1\n")
    git(a, "push", "-q", "origin", "main")
    git(top, "clone", "-q", up, b)
    commit(b, "g", "1\n")
    git(b, "push", "-q", "origin", "main")
    expect("clean, no upstream change seen yet", a, {
        "BRANCH": "main", "DETACHED": "0", "ROOT": a,
        "DIR": os.path.join(a, ".git"), "STAGED": "0", "UNSTAGED": "0",
        "UNTRACKED": "0", "UNMERGED": "0", "AHEAD": "0", "BEHIND": "0",
        "STASH": "0"})
    git(a, "fetch", "-q")
    commit(a, "h", "1\n")
    expect("ahead 1, behind 1", a, {"AHEAD": "1", "BEHIND": "1"})
    for i in range(2):
        with open(os.path.join(a, "f"), "a") as f:
            f.write("s%d\n" % i)
        git(a, "stash", "-q")
    # `# stash N` in porcelain v2 arrived in git 2.35. An older git prints
    # no header at all, and prompt_git4.c reads that as zero by design --
    # so on such a git the contract under test IS zero, not two.
    if git_version() >= (2, 35):
        expect("two stashes", a, {"STASH": "2", "UNSTAGED": "0"})
    else:
        expect("two stashes (git < 2.35 has no stash header: 0 by design)",
               a, {"STASH": "0", "UNSTAGED": "0"})
    with open(os.path.join(a, "f"), "a") as f:
        f.write("u\n")
    expect("unstaged", a, {"UNSTAGED": "1", "STAGED": "0"})
    with open(os.path.join(a, "h"), "a") as f:
        f.write("s\n")
    git(a, "add", "h")
    open(os.path.join(a, "new"), "w").close()
    expect("staged too, untracked not asked for", a, {
        "STAGED": "1", "UNSTAGED": "1", "UNTRACKED": "0"})
    expect("untracked when asked for", a, {"UNTRACKED": "1"}, True)
    c = os.path.join(top, "c")
    git(top, "init", "-q", "-b", "main", c)
    commit(c, "x", "base\n")
    git(c, "checkout", "-q", "-b", "side")
    commit(c, "x", "side\n")
    git(c, "checkout", "-q", "main")
    commit(c, "x", "main\n")
    subprocess.run(["git", "merge", "-q", "side"], cwd=c,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    expect("conflicted merge", c, {"UNMERGED": "1", "BRANCH": "main"})
    git(c, "merge", "--abort")
    got = expect("detached HEAD", c, {"DETACHED": "0"})
    git(c, "checkout", "-q", "--detach", "HEAD")
    got = expect("detached HEAD", c, {"DETACHED": "1"})
    check("HELLISH_GIT/detached branch is the short commit",
          len(got.get("BRANCH", "")) == 7, repr(got.get("BRANCH")))
    git(c, "checkout", "-q", "main")
    git(c, "-c", "protocol.file.allow=always", "submodule", "add", "-q",
        c, "mod")
    git(c, "commit", "-q", "-m", "sub")
    with open(os.path.join(c, "mod", "x"), "a") as f:
        f.write("dirty inside the submodule\n")
    expect("edits inside a submodule do not dirty the parent", c, {
        "UNSTAGED": "0", "STAGED": "0"})
    deep = os.path.join(c, "mod", "d1", "d2")
    os.makedirs(deep)
    got = expect("a submodule subdirectory still has its branch", deep, {
        "BRANCH": "main", "ROOT": os.path.join(c, "mod"), "UNSTAGED": "1"})
    check("HELLISH_GIT/submodule dir is the one under .git/modules",
          os.path.realpath(got.get("DIR", "")) ==
          os.path.join(c, ".git", "modules", "mod"), repr(got.get("DIR")))
    wt = os.path.join(top, "wt")
    git(c, "worktree", "add", "-q", "-b", "wtb", wt)
    got = expect("linked worktree", wt, {"BRANCH": "wtb", "ROOT": wt})
    check("HELLISH_GIT/worktree dir sits under worktrees/",
          "/worktrees/" in got.get("DIR", ""), repr(got.get("DIR")))
    expect("outside a repository", top, {
        "BRANCH": "", "ROOT": "", "DIR": "", "DETACHED": "0",
        "STAGED": "0", "UNSTAGED": "0", "UNTRACKED": "0", "UNMERGED": "0",
        "AHEAD": "0", "BEHIND": "0", "STASH": "0"})


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    if shutil.which("git"):
        vtop = tempfile.mkdtemp()
        git_vars_cases(os.path.realpath(vtop))
        shutil.rmtree(vtop, ignore_errors=True)
    zsh = find_zsh()
    if not zsh or not shutil.which("git"):
        print("skip: the zsh comparison needs zsh and git")
        return 1 if FAILS else 0
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

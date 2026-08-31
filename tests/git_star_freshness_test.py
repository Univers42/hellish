#!/usr/bin/env python3
"""The git dirty star must not outlive the state it describes.

The report: a prompt reading `on develop*` while `git status` in the same
shell, one line above, said "nothing to commit, working tree clean". The
star stayed for every prompt after that.

    ╰─❯ git push
        ... (success)
    ╰─❯ git status
        nothing to commit, working tree clean
    ╰─❯                     <- still `on develop*`
    ╰─❯                     <- still `on develop*`

Not a rendering bug. `git_dirty_cached` (prompt_git3.c) throttles the
async `git status -uno` with a TTL, and the TTL has two values:

    c->ttl = 3;
    if (c->at - c->spawned >= 1)
        c->ttl = 30;        <- a scan that took a second

The 30-second arm exists so a slow repo is not re-scanned continuously,
and it is a reasonable thing to want. But while it holds, the shell
answers "dirty?" from a cache **without checking whether anything
happened** -- so for half a minute after the working tree is cleaned, the
prompt keeps asserting modifications that are not there. On a big repo, or
a loaded machine, a one-second `git status` is ordinary.

The failure is therefore invisible on a small fast repo, which is why it
survived: every existing test uses one. So this file makes the slow scan
DETERMINISTIC instead of hoping for it -- a `git` shim earlier on PATH
sleeps before `status` and then execs the real git. No timing luck, no
big fixture repo, and the same reproduction on any machine.

Checks:
  1. a dirty tree shows the star (the test can detect a star at all)
  2. after the tree is cleaned BY A COMMAND IN THE SHELL, the very next
     prompt does not claim it is dirty      <- the report
  3. the same, with the slow-scan TTL engaged
  4. the reverse: a clean tree made dirty by a command grows a star
  5. `cd` to another repo never shows the previous repo's answer

Usage: python3 git_star_freshness_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []
GIT = shutil.which("git") or "/usr/bin/git"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def plain(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b).decode(errors="replace")


def git(repo, *args):
    subprocess.run([GIT, "-C", repo] + list(args), capture_output=True,
                   check=False)


def make_repo(root, branch="main"):
    os.makedirs(root, exist_ok=True)
    subprocess.run([GIT, "init", "-q", "-b", branch, root],
                   capture_output=True, check=False)
    git(root, "config", "user.email", "t@t")
    git(root, "config", "user.name", "t")
    with open(os.path.join(root, "f.txt"), "w") as f:
        f.write("one\n")
    git(root, "add", "-A")
    git(root, "commit", "-qm", "init")
    return root


def slow_git_dir(base, seconds="1.3"):
    """A `git` earlier on PATH that makes `status` take over a second.

    This is the whole reason the bug is reproducible here: the 30-second
    TTL arm is only taken when a scan is slow, and no fixture repo small
    enough to live in a test suite is ever slow. Shimming the clock is
    honest -- the shell cannot tell this from a genuinely large repo.
    """
    d = os.path.join(base, "slowbin")
    os.makedirs(d, exist_ok=True)
    p = os.path.join(d, "git")
    with open(p, "w") as f:
        f.write("#!/bin/sh\ncase \" $* \" in *\" status \"*) sleep %s ;; "
                "esac\nexec %s \"$@\"\n" % (seconds, GIT))
    os.chmod(p, 0o755)
    return d


class Shell:
    def __init__(self, cwd, path_prefix=None):
        path = "/usr/bin:/bin"
        if path_prefix:
            path = path_prefix + ":" + path
        env = {
            "HOME": tempfile.mkdtemp(prefix="hellish_star_home_"),
            # The `on branch*` segment lives in the RICH theme, which
            # stopped being the default -- ask for it by name.
            "PS1": "\\B",
            "PATH": path, "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "INPUTRC": "/dev/null", "HELLISH_NO_BANNER": "1",
            "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            try:
                os.chdir(cwd)
                os.execvp(SHELL, [SHELL])
            except BaseException:
                pass
            os._exit(127)
        self.read(1.6)

    def read(self, quiet=0.4, cap=12.0):
        out = b""
        last = time.time()
        end = last + cap
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    c = os.read(self.fd, 65536)
                except OSError:
                    break
                if not c:
                    break
                out += c
                last = time.time()
            elif time.time() - last >= quiet:
                break
        return plain(out)

    def send(self, s, quiet=0.4, cap=12.0):
        os.write(self.fd, s.encode())
        return self.read(quiet, cap)

    def settle(self, want, tries=10):
        """Render until the star agrees with `want`; return how many it took.

        The dirty scan is ASYNC by design -- the render never waits for
        git, so on a slow repo the answer legitimately arrives a render or
        two late (prompt_git3.c documents this trade). The contract is
        therefore CONVERGENCE, not immediacy: the prompt must tell the
        truth promptly, and `tries` is a bound comfortably under the 30s
        TTL that used to make it never converge at all.

        Returns the number of renders needed, or None if it never agreed.
        """
        for i in range(tries):
            got, line = self.star()
            if got == want:
                return i + 1
            self.last_line = line
        return None

    def star(self):
        """(star_present, the prompt line) for a fresh, empty prompt."""
        out = self.send("\n")
        for line in reversed(out.replace("\r", "").split("\n")):
            if " on " in line and "─" in line:
                return ("*" in line, line.strip()[:100])
        return (None, out[-140:])

    def close(self):
        try:
            os.kill(self.pid, signal.SIGKILL)
            os.waitpid(self.pid, 0)
        except OSError:
            pass


def dirty(repo):
    with open(os.path.join(repo, "f.txt"), "a") as f:
        f.write("change\n")


def run_case(label, base, slow):
    repo = make_repo(os.path.join(base, "repo_" + label))
    dirty(repo)
    prefix = slow_git_dir(base) if slow else None
    sh = Shell(repo, prefix)
    sh.last_line = ""
    try:
        n = sh.settle(True)
        check("%s: a modified tree shows the star" % label, n is not None,
              "never appeared; last prompt %r" % sh.last_line)
        # clean it the way the user did: a command run IN the shell
        sh.send("%s checkout -- .\n" % GIT, cap=20.0)
        n = sh.settle(False)
        check("%s: the star clears once the tree is clean" % label,
              n is not None,
              "still starred after 10 prompts: %r" % sh.last_line)
        # and stays cleared, rather than flickering back from the cache --
        # this is the half that the 30s TTL broke
        stuck = [sh.star()[0] for _ in range(4)]
        check("%s: it stays cleared on later prompts" % label,
              not any(stuck), "star came back: %r" % stuck)
        # the reverse direction has to work too, or "never dirty" would pass
        sh.send("printf 'x\\n' >> f.txt\n", cap=20.0)
        n = sh.settle(True)
        check("%s: a tree made dirty grows the star" % label,
              n is not None,
              "no star after modifying a tracked file: %r" % sh.last_line)
    finally:
        sh.close()


def test_cd_between_repos(base):
    a = make_repo(os.path.join(base, "clean_repo"))
    b = make_repo(os.path.join(base, "dirty_repo"), branch="develop")
    dirty(b)
    sh = Shell(b, slow_git_dir(base))
    sh.last_line = ""
    try:
        check("cd: the dirty repo starts starred",
              sh.settle(True) is not None,
              "never starred; last prompt %r" % sh.last_line)
        sh.send("cd %s\n" % a, cap=20.0)
        check("cd: a clean repo never inherits the previous repo's star",
              sh.star()[0] is False,
              "carried the star over: %r" % sh.last_line)
    finally:
        sh.close()


def main():
    base = tempfile.mkdtemp(prefix="hellish_gitstar_")
    try:
        # fast first: proves the harness detects a star at all, and that
        # the small-repo path (which always worked) still does.
        run_case("fast scan", base, slow=False)
        # the reported shape: a scan slow enough to take the 30s TTL arm.
        run_case("slow scan", base, slow=True)
        test_cd_between_repos(base)
    finally:
        shutil.rmtree(base, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

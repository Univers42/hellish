#!/usr/bin/env python3
"""Regression test: a zsh config pasted into a bash-dialect file -- issue #112.

A 42 student's ~/.zshrc, verbatim (tests/fixtures/issue112.zshrc):

    autoload -Uz vcs_info
    precmd() { vcs_info }
    zstyle ':vcs_info:git:*' ...
    PROMPT='...'
    RPROMPT='...'

pasted into ~/.hellishrc. Line 3 is the zsh idiom every prompt tutorial
uses: in zsh a bare `}` ALWAYS closes the group, in bash it is an argument
to vcs_info and the group stays open -- so the whole rest of the file was
swallowed and the shell said only "syntax error: unexpected end of file".

Three promises this pins, each of which was broken:

  * `emulate zsh` / `set -o zsh` ON THE FIRST LINE of a sourced file or a
    script arms the dialect for the REST OF THAT FILE. It did not: the
    whole file was lexed before the switch ran, exactly the "lexing ahead
    of execution" family that #105 fixed for shopt/alias/source. The
    dialect switch is a lexing hazard for the same reason shopt is.
  * `~/.config/hellish/rc.d/NN-name.zsh` is loaded, in the zsh dialect,
    between its .hsh siblings -- the same extension rule `source` uses,
    so a zsh-flavoured module needs no marker at all.
  * When the paste has no marker, the error names the cure. The bash
    dialect stays bash (`echo }` prints a brace, as the golden suite
    pins), but the "unexpected end of file" now carries a hint pointing
    at the `}` line and at `emulate zsh`.

Usage: python3 zsh_dialect_switch_test.py [/path/to/hellish]
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
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FIXTURE = os.path.join(ROOT, "tests", "fixtures", "issue112.zshrc")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)


ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")


def run(argv, cwd, timeout=15):
    try:
        p = subprocess.run(argv, cwd=cwd, env=ENV, capture_output=True,
                           timeout=timeout)
        return p.returncode, p.stdout.decode("utf-8", "replace"), \
            p.stderr.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return -1, "", "<timeout>"


def issue_rc():
    with open(FIXTURE, encoding="utf-8") as f:
        return f.read()


# ---- sourced files and scripts ----------------------------------------------
def switch_cases(d):
    body = issue_rc() + 'echo "LOADED=$?"\n'
    tail = '; type precmd; echo "P=[$PROMPT]"; echo "R=[$RPROMPT]"'
    for name, first in (("emulate zsh", "emulate zsh\n"),
                        ("set -o zsh", "set -o zsh\n")):
        write(os.path.join(d, "rc.hsh"), first + body)
        rc, out, err = run([SHELL, "-c", ". ./rc.hsh" + tail], d)
        check("sourced/%s on line 1 arms the rest of the file" % name,
              rc == 0 and "syntax error" not in err
              and "precmd is a function" in out,
              "rc=%d err=%r out=%r" % (rc, err[:160], out[:160]))
        check("sourced/%s: PROMPT and RPROMPT survive" % name,
              "P=[%F{cyan}" in out and "R=[%F{242}%n@%m %T%f]" in out,
              out[:200])
    write(os.path.join(d, "script.hsh"), "emulate zsh\n" + body)
    rc, out, err = run([SHELL, "script.hsh"], d)
    check("script/emulate zsh on line 1 arms the rest of the script",
          rc == 0 and "syntax error" not in err and "LOADED=0" in out,
          "rc=%d err=%r out=%r" % (rc, err[:160], out[:160]))
    # Not only line 1: a switch further down still governs what follows.
    write(os.path.join(d, "mid.hsh"),
          'A=1\necho "before"\nemulate zsh\nf() { echo late }\nf\n')
    rc, out, err = run([SHELL, "-c", ". ./mid.hsh"], d)
    check("sourced/a mid-file emulate zsh governs the lines after it",
          rc == 0 and out == "before\nlate\n", "rc=%d err=%r out=%r"
          % (rc, err[:120], out))


# ---- scope: pinned vs frame-local ------------------------------------------
def scope_cases(d):
    write(os.path.join(d, "probe.hsh"), "g() { echo probe }\ng\n")
    write(os.path.join(d, "pin.hsh"), "emulate zsh\nf() { echo pinned }\nf\n")
    rc, out, err = run([SHELL, "-c", ". ./pin.hsh; . ./probe.hsh"], d)
    check("scope/emulate zsh (no -L) stays on after the file ends",
          out == "pinned\nprobe\n" and "syntax error" not in err,
          "out=%r err=%r" % (out, err[:120]))
    write(os.path.join(d, "loc.hsh"),
          "emulate -L zsh\nf() { echo local }\nf\n")
    rc, out, err = run([SHELL, "-c", ". ./loc.hsh; . ./probe.hsh"], d)
    check("scope/emulate -L zsh is restored when the file ends",
          out == "local\n" and "syntax error" in err,
          "out=%r err=%r" % (out, err[:120]))
    # The gate holds: the bash dialect is still bash.
    rc, out, err = run([SHELL, "-c", "echo }"], d)
    check("gate/`echo }` still prints a brace in the bash dialect",
          out == "}\n", "out=%r err=%r" % (out, err[:80]))


# ---- the unmarked paste: same failure, but it names the cure ---------------
def hint_cases(d):
    write(os.path.join(d, "raw.hsh"), issue_rc())
    rc, out, err = run([SHELL, "-c", ". ./raw.hsh"], d)
    check("hint/unmarked zsh paste still fails like bash (status 2)",
          rc == 2 and "unexpected end of file" in err,
          "rc=%d err=%r" % (rc, err[:160]))
    check("hint/...and says `emulate zsh` is the cure",
          "emulate zsh" in err, "err=%r" % err[:300])
    check("hint/...pointing at the `}` line (line 3)",
          re.search(r"line 3\b", err) is not None, "err=%r" % err[:300])
    # A bash file with a REAL unterminated group gets no zsh hint: the
    # hint fires only when a bare `}` was seen inside an open group.
    write(os.path.join(d, "bash.hsh"), "f() {\n  echo hi\n")
    rc, out, err = run([SHELL, "-c", ". ./bash.hsh"], d)
    check("hint/a plain unterminated bash group gets no zsh hint",
          rc == 2 and "emulate zsh" not in err, "err=%r" % err[:200])


# ---- interactive: rc.d/*.zsh and ~/.hellishrc ------------------------------
def run_interactive(home, cwd, cmds, settle=1.2):
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "xterm-256color", "LANG": "C.UTF-8",
           "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
           "HELLISH_NO_UPDATE_CHECK": "1", "ASAN_OPTIONS": "detect_leaks=0"}
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(cwd)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "-i"])
        os._exit(127)
    out = b""

    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.15)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d

    drain(2.0)
    for c in cmds:
        os.write(fd, c.encode() + b"\n")
        drain(settle)
    os.write(fd, b"exit\n")
    drain(0.5)
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return out.decode("utf-8", "replace")


def strip(s):
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|[\x01\x02]",
                  "", s)


def rcd_cases():
    home = tempfile.mkdtemp()
    cfg = os.path.join(home, ".config", "hellish", "rc.d")
    write(os.path.join(cfg, "10-a.hsh"), 'ORD="${ORD}a"\n')
    write(os.path.join(cfg, "20-b.zsh"), 'g() { ORD="${ORD}b" }\ng\n')
    write(os.path.join(cfg, "30-c.hsh"), 'ORD="${ORD}c"\n')
    write(os.path.join(home, ".hellishrc"), 'ORD="${ORD}rc"\n')
    out = strip(run_interactive(home, home, ['echo "ORD=$ORD"']))
    check("rc.d/a .zsh module loads in the zsh dialect, in lexical order",
          "ORD=abcrc" in out and "syntax error" not in out,
          out[-300:])
    shutil.rmtree(home, ignore_errors=True)


def hellishrc_case():
    home = tempfile.mkdtemp()
    repo = os.path.join(home, "proj")
    os.makedirs(repo)
    subprocess.run(["git", "init", "-q", "-b", "main"], cwd=repo, check=True)
    subprocess.run(["git", "-c", "user.email=t@t", "-c", "user.name=t",
                    "commit", "-q", "--allow-empty", "-m", "init"],
                   cwd=repo, check=True)
    write(os.path.join(home, ".hellishrc"), "emulate zsh\n" + issue_rc())
    out = strip(run_interactive(home, repo, ["echo E2E-OK"]))
    check("hellishrc/issue #112 rc + `emulate zsh` loads with no error",
          "syntax error" not in out and "not supported" not in out
          and "E2E-OK" in out, out[-400:])
    check("hellishrc/...and the vcs_info prompt shows the branch",
          "on" in out and "main" in out, out[-400:])
    check("hellishrc/...with its %F{..} colours rendered, not leaked",
          "%F{" not in out and "{242}" not in out, out[-400:])
    shutil.rmtree(home, ignore_errors=True)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    d = tempfile.mkdtemp()
    try:
        switch_cases(d)
        scope_cases(d)
        hint_cases(d)
    finally:
        shutil.rmtree(d, ignore_errors=True)
    rcd_cases()
    if shutil.which("git"):
        hellishrc_case()
    print("\n%s" % ("ALL PASSED" if not FAILS else "%d FAILED: %s"
                    % (len(FAILS), ", ".join(FAILS))))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

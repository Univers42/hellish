#!/usr/bin/env python3
"""Regression test: the zsh prompt-customization vocabulary -- issue #91.

Every zsh prompt tutorial on the internet tells the user to write

    autoload -Uz colors && colors
    autoload -Uz vcs_info
    setopt PROMPT_SUBST
    zstyle ':vcs_info:git:*' formats ' %b'
    precmd() { vcs_info }
    PROMPT='%~ ${vcs_info_msg_0_} %# '
    RPROMPT='%D{%H:%M:%S}'

and a user pasted exactly that into ~/.hellishrc (issue #91). Every line
failed a different way: colors and vcs_info were "command not found"
(they live in zsh's fpath, which hellish does not ship), the precmd
function was recorded and never called, RPROMPT was silently ignored,
and -- the nastiest -- re-sourcing the rc died with
`syntax error near unexpected token --color=auto`, because the alias
scanner expanded aliases inside case PATTERNS the second time the
framework's own loader was parsed.

This test drives the whole tutorial rc on a real pty inside a real git
repository and asserts each promise separately, so a regression names its
victim. The alias-in-case parser bug also has 12 golden cases in
tests/alias_posix, diffed against the bash oracle on every push.

Usage: python3 prompt_zshrc_test.py [/path/to/hellish]
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
FAILS = []

RC = """HX_PROBE=0
precmd() { HX_PROBE=$((HX_PROBE + 1)); vcs_info; }
preexec() { HX_LAST_CMD="$1"; }
autoload -Uz colors && colors
autoload -Uz vcs_info
setopt PROMPT_SUBST
zstyle ':vcs_info:git:*' formats ' on:%b'
PROMPT='L1 %~${vcs_info_msg_0_}
L2 %# '
RPROMPT='%F{242}%D{%H:%M:%S}%f'
"""


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m",
                       name), flush=True)
    if not ok:
        if detail:
            print("       %s" % detail.replace("\n", "\n       "))
        FAILS.append(name)


def session(cwd, home, cmds, settle=1.4):
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

    drain(2.5)
    for c in cmds:
        os.write(fd, c + b"\n")
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


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    home = tempfile.mkdtemp(prefix="zshrc91-")
    repo = os.path.join(home, "proj")
    os.makedirs(repo)
    e = {"PATH": os.environ["PATH"], "HOME": home,
         "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null"}
    run = lambda *a: subprocess.run(["git", "-C", repo] + list(a), env=e,
                                    check=True, stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
    run("init", "-q", "-b", "mybranch")
    open(os.path.join(repo, "f"), "w").write("x\n")
    run("add", "f")
    run("-c", "user.email=t@t", "-c", "user.name=t", "commit", "-qm", "i")
    open(os.path.join(home, ".hellishrc"), "w").write(RC)

    out = session(repo, home, [b"echo PROBE=$HX_PROBE",
                               b"echo AGAIN=$HX_PROBE",
                               b'echo "LAST=[$HX_LAST_CMD]"',
                               b"printf '%s' \"${fg[green]}\" | od -c | head -1",
                               b"source ~/.hellishrc",
                               b"echo E2E-OK"])
    clean = strip(out)

    print("\n\033[1;36m▸\033[0m \033[1mthe tutorial rc, line by line\033[0m")
    bad = [l for l in clean.splitlines()
           if "not found" in l or "syntax error" in l or "not supported" in l]
    check("no error from any tutorial line (load AND re-source)",
          not bad, "\n".join(bad[:4]))
    check("the custom PROMPT renders (multi-line, %~)",
          "L1 " in clean and "\nL2 " in clean.replace("\r", ""),
          clean[-400:])
    check("precmd runs before every prompt (vcs_info fires)",
          re.search(r"PROBE=[1-9]", clean) is not None, clean[-400:])
    m1 = re.search(r"PROBE=(\d+)", clean)
    m2 = re.search(r"AGAIN=(\d+)", clean)
    check("precmd keeps running, once per prompt",
          m1 and m2 and int(m2.group(1)) > int(m1.group(1)),
          "counts %r %r" % (m1 and m1.group(1), m2 and m2.group(1)))
    # preexec fires for the line ABOUT TO RUN, so by the time this echo
    # executes, $1 was its own text -- zsh behaves identically.
    check("preexec receives the typed line as $1",
          'LAST=[echo "LAST=[$HX_LAST_CMD]"]' in clean, clean[-500:])
    check("vcs_info honours the zstyle format, branch included",
          " on:mybranch" in clean, clean[-400:])
    check("colors defined $fg[green] as an SGR sequence",
          re.search(r"033\s+\[\s+3\s+2\s+m", clean) is not None,
          clean[-400:])
    check("RPROMPT is drawn via cursor save/jump/restore",
          "\x1b[s" in out and "\x1b[u" in out and
          re.search(r"\d\d:\d\d:\d\d", clean) is not None, out[-300:])
    check("the shell still runs commands", "E2E-OK" in clean, clean[-200:])

    # bash-preexec owns the hook convention when loaded: hooks must not
    # fire twice. Its import guard is the detection.
    print("\n\033[1;36m▸\033[0m \033[1mno double fire under bash-preexec"
          "\033[0m")
    open(os.path.join(home, ".hellishrc"), "w").write(
        "N=0\nprecmd() { N=$((N + 1)); }\nbash_preexec_imported=defined\n"
        "PS1='BP$ '\n")
    out = session(repo, home, [b"echo N1=$N", b"echo N2=$N"])
    clean = strip(out)
    check("with bash-preexec claimed, native precmd stays quiet",
          "N1=0" in clean and "N2=0" in clean, clean[-300:])
    shutil.rmtree(home, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

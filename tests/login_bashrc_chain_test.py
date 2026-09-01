#!/usr/bin/env python3
"""The Debian login chain (issue #105): hellish as a login shell sources
/etc/profile -> ~/.profile -> ~/.bashrc -> bash_completion.

hellish sets BASH_VERSION, so the stock Debian ~/.profile sources
~/.bashrc, which sources /usr/share/bash-completion/bash_completion.
That file runs `shopt -s extglob` early and uses extglob patterns
hundreds of lines later — exec_string tokenized the whole file before
executing anything, so every ssh login printed
    hellish: syntax error near unexpected token `('
and aborted ~/.profile before its PATH block ran (=> `claude: command
not found` for anything living in ~/.local/bin).

This drives the whole chain in a pty against a throwaway HOME with the
stock Debian .profile/.bashrc shapes and a mini bash_completion carrying
the trigger constructs. Asserted: no syntax error, the completion file's
functions exist, the alias chain works, and the PATH block after the
bashrc sourcing still ran.

Usage: python3 login_bashrc_chain_test.py [/path/to/hellish]
"""
import os
import pty
import select
import shutil
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

PROFILE = """\
if [ -n "$BASH_VERSION" ]; then
    if [ -f "$HOME/.bashrc" ]; then
        . "$HOME/.bashrc"
    fi
fi
if [ -d "$HOME/.local/bin" ] ; then
    PATH="$HOME/.local/bin:$PATH"
fi
"""

BASHRC = """\
case $- in
    *i*) ;;
      *) return;;
esac
HISTCONTROL=ignoreboth
shopt -s histappend
shopt -s checkwinsize
alias ll='ls -alF'
if ! shopt -oq posix; then
  if [ -f "$HOME/mini_completion.sh" ]; then
    . "$HOME/mini_completion.sh"
  fi
fi
"""

# The bash_completion shape: extglob armed on line 2, used far below in
# case patterns and [[ ]] operands, plus an alias-then-use chain.
MINI_COMPLETION = """\
# mini bash_completion: the constructs that broke the real one
shopt -s extglob
_mini_rl_enabled()
{
    [[ "$(echo 'bell-style  on')" == *+([[:space:]])on* ]]
}
""" + "\n".join("# filler line %d" % i for i in range(60)) + """
_mini_dispatch()
{
    case "$1" in
        -?(\\[)+([a-zA-Z0-9?]))
            echo "extglob-case-ok" ;;
        *) : ;;
    esac
}
alias _mini_probe='echo mini-alias-ok'
_mini_probe
MINI_COMPLETION_LOADED=1
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def main():
    home = tempfile.mkdtemp(prefix="hellish105-")
    os.mkdir(os.path.join(home, ".local"))
    os.mkdir(os.path.join(home, ".local", "bin"))
    for name, body in ((".profile", PROFILE), (".bashrc", BASHRC),
                       ("mini_completion.sh", MINI_COMPLETION)):
        with open(os.path.join(home, name), "w") as f:
            f.write(body)

    pid, fd = pty.fork()
    if pid == 0:
        env = {"TERM": "xterm", "HOME": home, "USER": "tester",
               "LOGNAME": "tester", "PATH": "/usr/bin:/bin",
               "HELLISH_BANNER": "0", "HELLISH_NO_ANIM": "1",
               "HELLISH_NO_UPDATE_CHECK": "1"}
        os.execve(SHELL, ["-hellish", "--norc"], env)
        os._exit(127)
    time.sleep(1.0)
    for c in ('echo "LOADED=[$MINI_COMPLETION_LOADED]"',
              '_mini_rl_enabled && echo RL-OK',
              '_mini_dispatch -Word9',
              'll --version > /dev/null 2>&1; echo "ALIAS-LL=$?"',
              'case ":$PATH:" in *:"$HOME/.local/bin":*) echo P-OK;; '
              '*) echo P-MISSING;; esac',
              'exit'):
        os.write(fd, c.encode() + b"\r")
        time.sleep(0.4)
    out = b""
    deadline = time.time() + 8
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.3)
        if not r:
            try:
                wpid, _ = os.waitpid(pid, os.WNOHANG)
            except ChildProcessError:
                break
            if wpid:
                break
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            break
        if not d:
            break
        out += d
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    text = out.decode("utf-8", "replace")
    shutil.rmtree(home, ignore_errors=True)

    check("login chain: no syntax error", "syntax error" not in text,
          repr(text[:400]))
    check("bash_completion-shape file loaded", "LOADED=[1]" in text,
          repr(text[-400:]))
    check("[[ ]] extglob helper works", "RL-OK" in text, repr(text[-400:]))
    check("extglob case dispatch works", "extglob-case-ok" in text,
          repr(text[-400:]))
    check("alias-then-use inside sourced file", "mini-alias-ok" in text,
          repr(text[-400:]))
    check(".profile PATH block still ran", "P-OK" in text,
          repr(text[-400:]))
    if FAILS:
        print("\n%d FAILED: %s" % (len(FAILS), ", ".join(FAILS)))
        sys.exit(1)
    print("\nall clear")


main()

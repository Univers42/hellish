#!/usr/bin/env python3
"""A real-world login config, exercised from the get-go.

The rc a 42 account actually has is not the one hellishrc.example ships:
it is a ~/.zshrc that puts Homebrew on PATH, loads nvm, adds ~/.local/bin
and defines a few aliases -- and `install.sh --zshrc` (opt-in) loads that
file inside hellish (after.d/90-zshrc.zsh). The first person to say yes got

    manpath: warning: $PATH not set

at every prompt, and a shell that felt slow. The warning was a real,
universal shell bug wearing nvm's clothes: nvm_use reassigns PATH
(`PATH="$(nvm_change_path ...)"`) and only exports it AFTER running
`$(manpath)`; a plain assignment to an exported variable un-exported it
in hellish, so the child ran with no PATH at all. tests/export_attr pins
the shell rule against bash; this test pins the EXPERIENCE: a shell that
starts with that config, in the time it should, saying nothing.

The nvm pattern is vendored as a tiny stand-in (fake_nvm.sh), so the
test runs on every machine, not just where nvm is installed; when the
real ~/.nvm/nvm.sh is present it is loaded as well, for free.

Usage: python3 rc_realworld_test.py [/path/to/hellish]
"""
import os
import pty
import re
import select
import shutil
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

# What nvm_use does, reduced to the shape that bit: reassign an exported
# variable through a command substitution, run a child BEFORE exporting
# it again, then export. Every line of it is POSIX.
FAKE_NVM = r"""
nvm_change_path() { printf '%s' "$1"; }
nvm_use() {
    PATH="$(nvm_change_path "${PATH}" "/bin" "/nonexistent")"
    if [ -z "${MANPATH-}" ]; then
        MANPATH=$(sh -c 'if [ -z "${PATH-}" ]; then echo "manpath: warning: \$PATH not set" >&2; fi; echo /usr/share/man')
    fi
    MANPATH="$(nvm_change_path "${MANPATH}" "/share/man" "/nonexistent")"
    export MANPATH
    export PATH
    NVM_PROBE_CHILD_PATH="$(sh -c 'printf %s "${PATH:+set}"')"
}
nvm() { case "$1" in use) nvm_use ;; esac; }
nvm use default
"""

ZSHRC = """export PATH=$HOME/.brew/bin:$PATH
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \\. "$NVM_DIR/nvm.sh"  # This loads nvm
. "$HOME/fake_nvm.sh"
export PATH="$HOME/.local/bin:$PATH"
alias hello='echo from-zshrc'
"""

IMPORT = """if [ -f "$HOME/.zshrc" ]; then
\tHELLISH_EXECD=1
\texport HELLISH_EXECD
\tsource "$HOME/.zshrc"
fi
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)


def strip(s):
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|[\x01\x02]",
                  "", s)


def session(home, cmds):
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "xterm-256color", "LANG": "C.UTF-8",
           "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
           "HELLISH_NO_UPDATE_CHECK": "1", "ASAN_OPTIONS": "detect_leaks=0"}
    real_nvm = os.path.expanduser("~/.nvm/nvm.sh")
    if os.path.isfile(real_nvm):
        os.symlink(os.path.dirname(real_nvm), os.path.join(home, ".nvm"))
    t0 = time.time()
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "-i"])
        os._exit(127)
    out = b""
    first = None

    def drain(t):
        nonlocal out, first
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d
            if first is None and b"RCREAL$ " in out:
                first = time.time() - t0

    drain(3.0)
    for c in cmds:
        os.write(fd, c.encode() + b"\n")
        drain(1.0)
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
    return strip(out.decode("utf-8", "replace")), first


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    home = tempfile.mkdtemp(prefix="rcreal-")
    write(os.path.join(home, ".zshrc"), ZSHRC)
    write(os.path.join(home, "fake_nvm.sh"), FAKE_NVM)
    write(os.path.join(home, ".config", "hellish", "after.d", "90-zshrc.zsh"),
          IMPORT)
    # A known prompt, so "first prompt" is a byte we can wait for: unset,
    # hellish shows zsh's `host% `, which no fixed pattern should assume.
    write(os.path.join(home, ".hellishrc"), "PS1='RCREAL$ '\n")
    out, first = session(home, [
        "hello",
        'printf "CHILD-PATH=[%s]\\n" "$(sh -c \'printf %s "${PATH:+set}"\')"',
        'printf "PROBE=[%s]\\n" "$NVM_PROBE_CHILD_PATH"',
        'printf "EXPORTED=[%s]\\n" "$(printenv MANPATH)"',
    ])
    check("the config loads without a word on stderr (no manpath warning)",
          "warning" not in out and "not found" not in out
          and "syntax error" not in out, out[-500:])
    check("the alias from ~/.zshrc works", "from-zshrc" in out, out[-300:])
    check("a child run from the prompt sees PATH", "CHILD-PATH=[set]" in out,
          out[-300:])
    check("...and so did the child nvm ran BEFORE `export PATH`",
          "PROBE=[set]" in out, out[-300:])
    check("MANPATH reassigned inside a function stayed exported",
          "EXPORTED=[/usr/share/man]" in out, out[-300:])
    check("first prompt within 2s with the whole config loaded",
          first is not None and first < 2.0,
          "first prompt after %s" % (first,))
    shutil.rmtree(home, ignore_errors=True)
    print("\n%s" % ("ALL PASSED" if not FAILS else "%d FAILED: %s"
                    % (len(FAILS), ", ".join(FAILS))))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

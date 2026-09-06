#!/usr/bin/env python3
"""oh-my-zsh inside hellish, and the order the user's zsh config loads in.

Issue #114, from a 42 student whose ~/.zshrc is the oh-my-zsh template:

    Error: Oh My Zsh can't be loaded from: hellish. You need to run zsh
    instead.  Here's the process tree: ...

at every start.  The installer's bridge loads ~/.zshrc inside hellish in
the zsh dialect, and `source $ZSH/oh-my-zsh.sh` ran oh-my-zsh's guard,
which refuses any shell without $ZSH_VERSION -- so the one line every
such rc has produced a screen of red, and the plugins it named (git,
in every template) never loaded.

What this pins, on a fake $ZSH tree so it needs no network and no real
oh-my-zsh (both are optional extras below):

  * `source .../oh-my-zsh.sh` runs the shim: the plugins named by
    `plugins=(...)` load from $ZSH_CUSTOM/plugins then $ZSH/plugins,
    then $ZSH_CUSTOM/*.zsh -- and the guard never speaks;
  * plugins that are the zsh line editor (zsh-autosuggestions and its
    kind) are skipped by name, silently;
  * `is-at-least` exists, so the git plugin's version checks are quiet;
    `aliases[name]=value` defines an alias, as in zsh;
  * the import lives in after.d and so loads AFTER ~/.hellishrc: an
    oh-my-zsh alias and a framework function of the same name (`gwip`)
    no longer collide -- alias first, function second is a syntax error
    in every shell -- and the alias wins, as it does in zsh;
  * nothing zsh-only prints "not supported" during the shim, and the
    rest of the rc (aliases, exports, `[[ == (a|b) ]]`, precmd) runs.

Usage: python3 omz_bridge_test.py [/path/to/hellish]
       OMZ_SRC=<dir>   also run the real oh-my-zsh checkout at <dir>
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

# oh-my-zsh.sh's guard, verbatim in spirit: the shim must keep it from
# ever running.  LIB-LOADED stands for everything after it (lib/, themes,
# compinit) that hellish deliberately does not load.
FAKE_OMZ = r'''
[ -n "$ZSH_VERSION" ] || {
  echo "Error: Oh My Zsh can't be loaded from: hellish. You need to run zsh instead." >&2
  return 1
}
echo LIB-LOADED
'''

GIT_PLUGIN = r'''
autoload -Uz is-at-least
alias gst='git status'
alias gwip='echo omz-gwip'
if is-at-least 2.8 2.34.1; then GIT_NEW=1; fi
local old_name new_name
for old_name new_name (
  current_branch  git_current_branch
); do
  aliases[$old_name]="echo deprecated; $new_name"
done
unset old_name new_name
compdef _git gst=git-status
'''

ZSHRC = r'''
export ZSH="$HOME/.oh-my-zsh"
ZSH_THEME="fino"
plugins=(
	git
	zsh-autosuggestions
	mine
	nosuchplugin
)
source $ZSH/oh-my-zsh.sh
alias compilef="cc -Wall -Wextra -Werror -g3"
export PATH="$HOME/.brew/bin:$PATH"
[[ $TERM == (xterm*|screen*) ]] && FROM_GROUP=1
precmd() { : }
unsetopt beep
'''

BRIDGE = r'''
if [ -f "$HOME/.zshrc" ]; then
	HELLISH_EXECD=1
	export HELLISH_EXECD
	source "$HOME/.zshrc"
fi
'''

# What the plugin framework does: a function named like an oh-my-zsh
# alias.  In rc.d order this was `<alias body>() {` -- a syntax error.
HELLISHRC = r'''
gwip() { echo fw-gwip; }
alias fwonly='echo fw-only'
'''


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)


def env_for(home):
    return {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "XDG_CONFIG_HOME": os.path.join(home, ".config"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "HELLISH_BANNER": "0", "HELLISH_NO_UPDATE_CHECK": "1",
            "ASAN_OPTIONS": "detect_leaks=0"}


def run(home, argv):
    p = subprocess.run(argv, cwd=home, env=env_for(home),
                       capture_output=True, text=True, timeout=60)
    return p.returncode, p.stdout, p.stderr


def run_interactive(home, cmds, settle=1.0):
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env_for(home))
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
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|[\x01\x02]",
                  "", out.decode("utf-8", "replace"))


def build_home(d):
    home = os.path.join(d, "home")
    zsh = os.path.join(home, ".oh-my-zsh")
    write(os.path.join(zsh, "oh-my-zsh.sh"), FAKE_OMZ)
    write(os.path.join(zsh, "plugins", "git", "git.plugin.zsh"), GIT_PLUGIN)
    write(os.path.join(zsh, "custom", "plugins", "zsh-autosuggestions",
                       "zsh-autosuggestions.plugin.zsh"),
          "echo SHOULD-NOT-RUN\nzle -N _zsh_autosuggest_x\n")
    write(os.path.join(zsh, "custom", "plugins", "mine", "mine.plugin.zsh"),
          "alias frommine='echo mine-ok'\n")
    write(os.path.join(zsh, "custom", "extra.zsh"),
          "alias fromcustom='echo custom-ok'\n")
    write(os.path.join(home, ".zshrc"), ZSHRC)
    write(os.path.join(home, ".hellishrc"), HELLISHRC)
    write(os.path.join(home, ".config", "hellish", "after.d",
                       "90-zshrc.zsh"), BRIDGE)
    return home


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    d = tempfile.mkdtemp(prefix="hellish-omz-")
    try:
        home = build_home(d)
        out = run_interactive(home, [
            "type gst", "gwip", "frommine", "fromcustom", "fwonly",
            "type compilef", "echo group=$FROM_GROUP new=$GIT_NEW",
            "type current_branch", "echo ZSH=$ZSH", "echo done-$?"])
        check("guard/the oh-my-zsh guard never speaks",
              "can't be loaded" not in out, "out=%r" % out[:500])
        check("guard/lib, themes, compinit are not loaded",
              "LIB-LOADED" not in out, "out=%r" % out[:500])
        check("plugins/git plugin's aliases are defined",
              "gst is aliased to `git status'" in out, "out=%r" % out[:800])
        check("plugins/custom plugin loads", "mine-ok" in out,
              "out=%r" % out[:800])
        check("plugins/$ZSH_CUSTOM/*.zsh loads", "custom-ok" in out,
              "out=%r" % out[:800])
        check("plugins/ZLE-only plugin skipped silently",
              "SHOULD-NOT-RUN" not in out and "not supported" not in out,
              "out=%r" % out[:800])
        check("plugins/is-at-least answered (no 'command not found')",
              "new=1" in out and "is-at-least" not in out,
              "out=%r" % out[:800])
        check("plugins/aliases[name]=value defines an alias",
              "current_branch is aliased to" in out,
              "out=%r" % out[:800])
        check("order/after.d loads after ~/.hellishrc: no gwip collision",
              "syntax error" not in out, "out=%r" % out[:800])
        check("order/the oh-my-zsh alias wins over the framework function",
              "omz-gwip" in out and "fw-gwip" not in out,
              "out=%r" % out[:800])
        check("order/~/.hellishrc still loaded", "fw-only" in out,
              "out=%r" % out[:800])
        check("rest/the user's own aliases after the source line",
              "compilef is aliased" in out, "out=%r" % out[:800])
        check("rest/[[ == (a|b) ]] and unsetopt beep are quiet",
              "group=1" in out and "no such option" not in out
              and "missing" not in out, "out=%r" % out[:800])
        check("rest/$ZSH is set", "ZSH=%s" % os.path.join(home, ".oh-my-zsh")
              in out, "out=%r" % out[:800])
        check("rest/no error of any kind reached the terminal",
              not re.search(r"hellish: .*(error|not found)", out),
              "out=%r" % out[:800])
        check("rest/the shell is alive", "done-0" in out,
              "out=%r" % out[:800])

        # The shim is by file name, in either dialect, from a script too.
        rc, sout, serr = run(home, [SHELL, "-c",
                                    "plugins=git; source ~/.oh-my-zsh/oh-my-zsh.sh; "
                                    "type gst; echo rc=$?"])
        check("script/works from -c with a scalar plugins=",
              "gst is aliased" in sout and "rc=0" in sout and serr == "",
              "out=%r err=%r" % (sout, serr))
        # A missing oh-my-zsh is still a missing file.
        rc, sout, serr = run(home, [SHELL, "-c",
                                    "source /nonexistent/oh-my-zsh.sh"])
        check("script/a missing oh-my-zsh.sh is reported as missing",
              rc != 0 and "No such file" in serr, "err=%r" % serr)

        # Optional: the real thing, when a checkout is at hand.
        real = os.environ.get("OMZ_SRC")
        if real and os.path.isfile(os.path.join(real, "oh-my-zsh.sh")):
            home2 = os.path.join(d, "home2")
            shutil.copytree(real, os.path.join(home2, ".oh-my-zsh"),
                            symlinks=True)
            write(os.path.join(home2, ".zshrc"),
                  'export ZSH="$HOME/.oh-my-zsh"\nplugins=(git sudo extract)\n'
                  'source $ZSH/oh-my-zsh.sh\n')
            write(os.path.join(home2, ".config", "hellish", "after.d",
                               "90-zshrc.zsh"), BRIDGE)
            out = run_interactive(home2, ["type gst", "type extract",
                                          "echo done-$?"])
            check("real/oh-my-zsh git+sudo+extract load with no noise",
                  "gst is aliased" in out and "extract is a function" in out
                  and not re.search(r"hellish: .*(error|not found|"
                                    r"not supported|invalid)", out),
                  "out=%r" % out[:1200])
    finally:
        shutil.rmtree(d, ignore_errors=True)
    print("\n%d failure(s)" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

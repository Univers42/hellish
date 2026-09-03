#!/usr/bin/env python3
"""The zsh-rc constructs a fresh 42 ~/.zshrc runs into -- issue #113's
neighbourhood, pinned one by one.

Every line here is something a tutorial ~/.zshrc or an oh-my-zsh file
writes, and every one of them failed in the dialect on the day #113 was
filed, most of them loudly at every shell start:

    [[ $TERM == (xterm*|screen*) ]]     "[[: missing `]]'" then
                                         "screen*: command not found"
    for f (*.zsh) source $f              syntax error near `source'
    local x=1  (at file scope)           "can only be used in a function"
    alias -g L='| less'                  "alias: -g: not found"
    unsetopt beep                        "no such option: beep"
    f() ( ... )                          "unexpected end of file" (bash too)
    is-at-least 2.8 $v                   "command not found"

Each is run from a .zsh file (the extension arms the dialect BEFORE the
file is lexed, which a `-c 'emulate zsh; ...'` one-liner cannot do), with
the expected stdout and an empty stderr.  When a zsh is on PATH the stdout
is also diffed against it, so the expectation is zsh's and not ours.

Usage: python3 zsh_rc_syntax_test.py [/path/to/hellish]
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

# (name, script, expected stdout, compare with zsh?)
CASES = [
    ("group/front, bare",
     'x=abc; [[ $x == (abc|x) ]] && echo yes\n', "yes\n", True),
    ("group/front, then more pattern",
     'x=abcdef; [[ $x == (abc|x)* ]] && echo yes\n', "yes\n", True),
    ("group/three alternatives with globs",
     'TERM=screen-256color; [[ $TERM == (xterm*|screen*|dumb) ]] && echo yes\n',
     "yes\n", True),
    ("group/option-looking alternatives",
     'set -- -h; [[ "$1" == (-h|--help) ]] && echo help\n', "help\n", True),
    ("group/mid-word still works",
     '[[ abc == a(b|z)c ]] && echo yes\n', "yes\n", True),
    ("group/[[ grouping parens are not a pattern",
     'x=abc; [[ ( $x == abc ) && ( 1 == 1 ) ]] && echo yes\n', "yes\n", True),
    ("group/a subshell is still a subshell",
     '(echo a|tr a b)\n', "b\n", True),
    ("group/case keeps a mid-word group",
     'case abc in a(b|z)c) echo yes;; esac\n', "yes\n", True),
    ("for/short body, one command",
     'for x (a b) echo $x\n', "a\nb\n", True),
    ("for/short body, brace group",
     'for x (a b) { echo $x }\n', "a\nb\n", True),
    ("for/short body on the next line",
     'for x (a b)\n  echo $x\n', "a\nb\n", True),
    ("for/short body runs a function by name",
     'foo() { echo infn; }\nfor f (foo) $f\n', "infn\n", True),
    ("for/do-done after the paren list",
     'for x (a b); do echo $x; done\n', "a\nb\n", True),
    ("local/at file scope is a plain assignment",
     'local q=1; echo local=$q\n', "local=1\n", True),
    ("local/-a at file scope",
     'local -a arr=(x y); echo ${#arr[@]}\n', "2\n", True),
    ("alias/-g defines the alias",
     "alias -g L='| head -1'; alias L | grep -c head\n", "1\n", False),
    ("alias/-s is accepted silently",
     "alias -s txt=cat; echo ok\n", "ok\n", True),
    ("setopt/every real zsh option is accepted",
     "unsetopt beep; setopt hist_expire_dups_first flowcontrol "
     "long_list_jobs no_beep NO_HUP; echo ok\n", "ok\n", True),
    ("setopt/a typo is still an error",
     "setopt not_an_option 2>/dev/null; echo rc=$?\n", "rc=1\n", True),
    ("funcbody/subshell body",
     'f() ( echo sub ); f\n', "sub\n", True),
    ("funcbody/while body",
     'f() while false; do :; done; f; echo w-ok\n', "w-ok\n", True),
    ("is-at-least/explicit versions",
     'is-at-least 2.8 2.34.1 && echo v1; is-at-least 2.34.1 2.8 || echo v2; '
     'is-at-least 2.30 2.30 && echo eq; is-at-least 5.0.1 5.0 || echo lt\n',
     "v1\nv2\neq\nlt\n", False),
    ("aliases/aliases[name]=value defines an alias",
     'aliases[gx]="echo via-aliases"; alias gx | grep -c via\n', "1\n", False),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(argv, cwd):
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "HOME": cwd,
           "TERM": "xterm", "LANG": "C.UTF-8",
           "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_BANNER": "0",
           "ASAN_OPTIONS": "detect_leaks=0"}
    p = subprocess.run(argv, cwd=cwd, env=env, capture_output=True,
                       text=True, timeout=30)
    return p.returncode, p.stdout, p.stderr


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    zsh = shutil.which("zsh")
    d = tempfile.mkdtemp(prefix="hellish-zshrc-")
    try:
        for name, script, want, vs_zsh in CASES:
            path = os.path.join(d, "case.zsh")
            with open(path, "w") as f:
                f.write(script)
            rc, out, err = run([SHELL, "-c", "source case.zsh"], d)
            check(name, out == want and err == "",
                  "out=%r err=%r want=%r" % (out, err, want))
            if zsh and vs_zsh:
                _, zout, _ = run([zsh, "-c", "source case.zsh"], d)
                check(name + " (zsh agrees)", zout == want,
                      "zsh=%r want=%r" % (zout, want))
    finally:
        shutil.rmtree(d, ignore_errors=True)
    print("\n%d failure(s)" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

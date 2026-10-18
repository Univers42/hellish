#!/usr/bin/env python3
"""The prompt example the docs give is the one that works (issue #134, 2, 8).

USER_DOC.md and hellishrc.example both say that `$(...)` in a prompt is
never run, and show the way to get a computed segment instead: a variable
set by a HELLISH_PRECMD_FUNCS hook. Documentation that no longer matches
the binary is what section 8 of the issue was about, so this runs the
snippet exactly as USER_DOC.md prints it -- read from the file, not
copied here -- and checks that:
  * hellishrc.example carries the same snippet;
  * the hook's segment shows up in the prompt, and follows a `cd` into a
    git repository on the next prompt;
  * `$(...)` and backquotes in PS1 stay on screen as written.

Usage: python3 prompt_doc_example_test.py [/path/to/hellish]
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "helpers"))
from ptyexpect import Tty  # noqa: E402

ROOT = os.path.dirname(HERE)
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
T = 8.0


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def doc_snippet():
    """The ```sh block that follows 'For a computed segment' in USER_DOC."""
    with open(os.path.join(ROOT, "USER_DOC.md")) as f:
        text = f.read()
    m = re.search(r"For a computed segment.*?```sh\n(.*?)```", text, re.S)
    return m.group(1) if m else None


def example_snippet():
    """The same lines as hellishrc.example comments them: '#     code'."""
    with open(os.path.join(ROOT, "hellishrc.example")) as f:
        lines = f.read().split("\n")
    out = [ln[6:] for ln in lines if ln.startswith("#     ")]
    return "\n".join(out)


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    snip = doc_snippet()
    check("doc/USER_DOC-has-the-snippet", snip is not None)
    if snip is None:
        return 1
    ex = example_snippet()
    check("doc/hellishrc.example-has-the-same-snippet",
          all(ln in ex for ln in snip.strip().split("\n")),
          "snippet=%r" % snip)
    if not shutil.which("git"):
        print("skip: no git")
        return 1 if FAILS else 0
    t = Tty(SHELL, rc=snip + "PS2='> '\n")
    try:
        ok = t.expect(b"~ ", T)
        check("prompt/renders-outside-a-repo", ok, "tail=%r" % t.out[-120:])
        repo = os.path.join(t.home, "r")
        os.mkdir(repo)
        subprocess.run(["git", "init", "-q", repo], check=True)
        subprocess.run(["git", "-C", repo, "checkout", "-q", "-b", "topic"],
                       check=True)
        t.send("cd r\r")
        check("prompt/hook-segment-follows-cd", t.expect(b"(topic) ~/r ", T),
              "tail=%r" % t.out[-160:])
        t.send("PS1='[$(echo CS)] [`echo BQ`] '\r")
        check("prompt/command-substitution-is-not-run",
              t.expect(b"[$(echo CS)] [`echo BQ`] ", T),
              "tail=%r" % t.out[-160:])
    finally:
        t.close()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

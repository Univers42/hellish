#!/usr/bin/env python3
"""Parity: the zsh prompt language, diffed byte-for-byte against zsh 5.9.

`print -P` renders a prompt-format string through the same engine the live
prompt uses, and zsh has the same builtin -- so the entire escape set can
be diffed golden-style, with no pty and no screen scraping:

    hellish -c 'set -o zsh; print -P -- FMT'   ==   zsh -c 'print -P -- FMT'

This is the test that keeps "interprets the zsh syntax" an actual property
rather than a claim. The reference behaviours were MEASURED before being
implemented, and several contradicted the manual-derived guess: %# renders
`%` (not the bash $), an unknown escape renders NOTHING, a trailing lone %
is dropped, %b is a full SGR reset, %F{red} is the classic \\e[31m rather
than 38;5;1, and negative path counts take LEADING components.

Known, deliberate non-goals (evaluate false / render empty, documented in
src/infrastructure/prompt_zsh5.c): the clock-comparison conditionals
(t/T/d/D/w), %(S..), %(g..), %(l..), %(v..), and %(_..). Nothing here
tests them against the oracle because the answer would differ; they are
listed so the gap is a decision, not a surprise.

Usage: python3 zsh_prompt_parity_test.py [/path/to/hellish]
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

TIMEY = ("%T", "%t", "%@", "%*", "%D", "%W", "%w")


def find_zsh():
    env = os.environ.get("ZSH_ORACLE")
    if env and os.access(env, os.X_OK):
        return env
    home = os.path.expanduser("~/zsh-5.9/bin/zsh")
    if os.access(home, os.X_OK):
        return home
    return None


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m",
                       name), flush=True)
    if not ok:
        if detail:
            print("       %s" % detail.replace("\n", "\n       "))
        FAILS.append(name)


class Rig:
    def __init__(self, zsh):
        self.zsh = zsh
        self.tmp = tempfile.mkdtemp(prefix="zparity-")
        self.home = os.path.join(self.tmp, "home")
        self.deep = os.path.join(self.home, "proj", "deep")
        os.makedirs(self.deep)
        self.env = {
            "HOME": self.home, "PATH": os.environ.get("PATH", "/usr/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8", "SHLVL": "4",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1",
        }

    def render(self, shell, script, cwd):
        r = subprocess.run([shell, "-c", script], capture_output=True,
                          env=self.env, cwd=cwd, timeout=20)
        return r.stdout, r.stderr

    def compare(self, name, fmt, cwd=None, pre=""):
        """One format through both shells; time-bearing ones get retries,
        because the clock may tick between the two renders."""
        cwd = cwd or self.deep
        # `set -o zsh` shares the first line so LINENO agrees between the
        # two scripts -- %i is under test.
        h_script = "set -o zsh; %sprint -P -- '%s'" % (pre or "true\n", fmt)
        z_script = "%sprint -P -- '%s'" % (pre or "true\n", fmt)
        tries = 3 if any(t in fmt for t in TIMEY) else 1
        for _ in range(tries):
            ho, he = self.render(SHELL, h_script, cwd)
            zo, ze = self.render(self.zsh, z_script, cwd)
            if ho == zo:
                break
        check(name, ho == zo,
              "fmt   %r\nhellish %r (stderr %r)\nzsh     %r (stderr %r)"
              % (fmt, ho, he[-120:], zo, ze[-120:]))


def main():
    zsh = find_zsh()
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    if not zsh:
        print("skip: no zsh oracle -- run `make zsh-oracle` to enable this "
              "suite")
        sys.exit(0)
    rig = Rig(zsh)
    root = "/"
    usr = "/usr/local/bin"

    print("\n\033[1;36m▸\033[0m \033[1midentity and counters\033[0m")
    for fmt in ("%n", "%m", "%M", "%#", "%?", "%j", "%h", "%!", "%L", "%i",
                "%e", "%l", "%y", "[%_]", "[%^]"):
        rig.compare(fmt, fmt)
    rig.compare("%? after false", "%?", pre="false\n")
    rig.compare("%? in text", "st=[%?] done", pre="false\n")

    print("\n\033[1;36m▸\033[0m \033[1mpaths, from four directories\033[0m")
    for fmt in ("%~", "%d", "%/", "%c", "%C", "%.", "%1~", "%2~", "%3~",
                "%-1~", "%1d", "%2d", "%-1d", "%-2d", "%2c"):
        rig.compare(fmt + " (deep in HOME)", fmt)
        rig.compare(fmt + " (at HOME)", fmt, cwd=rig.home)
        rig.compare(fmt + " (at /)", fmt, cwd=root)
        rig.compare(fmt + " (outside HOME)", fmt, cwd=usr)

    print("\n\033[1;36m▸\033[0m \033[1meffects and colours\033[0m")
    for fmt in ("%B*%b", "%U*%u", "%S*%s", "%E*", "%{RAW%}*",
                "%F{red}*%f", "%F{green}*%f", "%F{blue}*%f", "%F{81}*%f",
                "%F{236}*%f", "%K{yellow}*%k", "%K{202}*%k",
                "%F{#ff8800}*%f", "%F{#000000}*%f", "%B%F{cyan}x%f%b"):
        rig.compare(fmt, fmt)

    print("\n\033[1;36m▸\033[0m \033[1mthe clock family\033[0m")
    for fmt in ("%T", "%t", "%@", "%*", "%D", "%W", "%w",
                "%D{%Y-%m-%d}", "%D{%H}", "%D{}"):
        rig.compare(fmt, fmt)

    print("\n\033[1;36m▸\033[0m \033[1mconditionals\033[0m")
    for fmt in ("%(?.yes.no)", "%(1?.yes.no)", "%(!.root.user)",
                "%(1j.busy.idle)", "%(2L.deep.shallow)", "%(9L.deep.shallow)",
                "%(3C.long.short)", "%(3c.long.short)", "%(2~.long.short)",
                "%(?.a%(!.b.c)d.e)", "%(?:A:B)", "%(?.only-true)",
                "%(1?..only-false)"):
        rig.compare(fmt, fmt)
    rig.compare("%(?..) after false", "%(?.good.bad)", pre="false\n")
    rig.compare("%(2?..) exact code", "%(2?.two.other)",
                pre="sh -c 'exit 2'\n")

    print("\n\033[1;36m▸\033[0m \033[1mtruncation\033[0m")
    for fmt in ("%10<...<abcdefghijklmnop%<<END",
                "%10>...>abcdefghijklmnop%>>END",
                "%10<..<short%<<END", "%5<..<%~", "[%8<..<%~]",
                "%20<...<%~%<< done"):
        rig.compare(fmt, fmt)

    print("\n\033[1;36m▸\033[0m \033[1medge cases and real themes\033[0m")
    for fmt in ("[%Z]", "[%Q]", "end%", "%%", "a%%b", "100%% done",
                "%(", "%)", "%v", "[%v][%2v][%3v]"):
        rig.compare(fmt, fmt)
    rig.compare("psvar set", "[%v][%2v][%3v]",
                pre="psvar=(alpha beta)\n" if True else "")
    rig.compare("robbyrussell shape",
                "%(?:OK :NO )%F{cyan}%c%f $ ")
    rig.compare("two-line theme", "%F{blue}%~%f %# ")
    shutil.rmtree(rig.tmp, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

#!/usr/bin/env python3
"""Compatibility matrix: legacy bash PS1 rc files vs the zsh PROMPT syntax.

The fear this test answers: users carry years-old ~/.hellishrc files whose
PS1 is pure bash -- backslash escapes, raw ANSI codes, \\[ \\] width
markers -- and issue #69 added a second prompt language on top (zsh-style
`%` escapes via PROMPT). Two languages over one screen is exactly where a
"harmless" change breaks a prompt someone built five years ago.

The contract under test, from prompt.c / prompt_zsh.c:

  * PROMPT set and non-empty  -> exact zsh semantics (unknown escapes
    consumed, oracle parity), backslash escapes as a bonus;
  * else PS1                  -> BILINGUAL: bash escapes and every KNOWN
    zsh escape both render, because PS1 habits do not migrate -- while an
    unknown or malformed `%` sequence stays literal and $()/${}/\D{}
    spans are copied verbatim, so a legacy percent is never eaten;
  * PS1 under `set -o zsh`    -> exact zsh, like zsh's own PS1;
  * else                      -> the built-in theme.

Every case below is a complete rc file, written to its own throwaway HOME
and SOURCED by a real interactive session on a real pty -- the only way
~/.hellishrc is ever read. Three things are asserted per case:

  1. the session still works: a sentinel command runs and answers;
  2. nothing diagnostic reaches the screen -- no "hellish:", no ASan, no
     "command not found" fallout from a prompt string;
  3. the escapes were consumed (their raw spelling is gone from the
     output) or deliberately preserved (the literal-% guarantee, and
     $(cmd), which the renderer refuses to execute by design).

Values that vary by machine (user, host, time) are pinned with sentinel
brackets around the escape instead of guessing the value:  PS1='<<\\u>>'
proves expansion by the survival of the brackets and the disappearance of
the spelling, on any box and in any container.

Usage: python3 prompt_compat_matrix_test.py [/path/to/hellish]
"""
import os
import pty
import select
import shutil
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
LOCK = threading.Lock()

# Output that means the rc file broke something, whatever the case is.
# "hellish:" is the shell's own diagnostic prefix: a prompt string must
# never produce one. The ASan strings catch a memory bug surfacing as
# text; "command not found" catches an rc line being mis-parsed into an
# execution attempt.
GLOBAL_REJECTS = ["hellish:", "AddressSanitizer", "LeakSanitizer",
                  "Segmentation", "command not found", "syntax error"]


def check(name, ok, detail=""):
    with LOCK:
        print("  %s %s" % ("\033[32mok\033[0m  " if ok
                           else "\033[31mFAIL\033[0m", name), flush=True)
        if not ok:
            if detail:
                print("       %s" % detail.replace("\n", "\n       "))
            FAILS.append(name)


def run_case(rc):
    """One interactive session in a throwaway HOME that holds `rc`."""
    home = tempfile.mkdtemp(prefix="promptrc-")
    with open(os.path.join(home, ".hellishrc"), "w") as f:
        f.write(rc + "\n")
    env = dict(os.environ, HOME=home, TERM="dumb", HELLISH_NO_BANNER="1",
               HELLISH_NO_ANIM="1", HELLISH_NO_UPDATE_CHECK="1")
    for v in ("PS1", "PROMPT", "PROMPT_COMMAND"):
        env.pop(v, None)
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.execve(SHELL, [SHELL, "-i"], env)
        os._exit(1)
    out = b""

    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d

    drain(1.2)
    os.write(fd, b"echo E2E-$((2000 + 26))\n")
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
    shutil.rmtree(home, ignore_errors=True)
    return out.decode("utf-8", "replace")


# (name, rc file content, must-appear, must-not-appear). The sentinel
# command's answer E2E-2026 is an implicit must-appear on every case.
CASES = [
    # ── the classics every distro ever shipped ─────────────────────────
    ("classic user@host", r"PS1='<<\u>>@\h:\w\$ '", ["<<", ">>@"],
     [r"\u", r"\h", r"\w"]),
    ("ubuntu default (debian_chroot + color)",
     "PS1='${debian_chroot:+($debian_chroot)}\\[\\033[01;32m\\]\\u@\\h"
     "\\[\\033[00m\\]:\\[\\033[01;34m\\]\\w\\[\\033[00m\\]\\$ '",
     ["\x1b[01;32m"], [r"\u", r"\w", r"\[", r"\]", r"\033"]),
    ("bare dollar", r"PS1='\$ '", ["$ "], []),
    ("basename only", r"PS1='\W \$ '", [], [r"\W"]),
    ("tilde for HOME", r"PS1='(\w)\$ '", ["(~)"], [r"\w"]),
    ("root-style hash guard", r"PS1='[\u@\h \W]\$ '", ["["], [r"\W"]),
    ("suse style", r"PS1='\u@\h:\w> '", [], [r"\u"]),
    # ── every time/date escape at once ─────────────────────────────────
    ("time 24h", r"PS1='[\t] \$ '", [], [r"\t]"]),
    ("time 12h + ampm", r"PS1='\T \@ \$ '", [], [r"\T", r"\@"]),
    # \A is the ONE deliberate divergence: hellishrc.example documents it
    # as the animation glyph, shadowing bash's 24-hour clock. It must
    # still be CONSUMED (rendering nothing with animation off), never
    # printed raw.
    ("date + short time", r"PS1='\d \A \$ '", [], [r"\d", r"\A"]),
    ("carriage return escape", r"PS1='\r\$ '", [], [r"\r\$"]),
    # ── identity / metadata escapes ────────────────────────────────────
    ("shell name and version", r"PS1='\s-\v (\V) \$ '", [], [r"\s", r"\V"]),
    # \l varies with the pty device; jobs:history:command are exact -- a
    # fresh HOME has an empty history file, so the first prompt is number 1.
    ("tty + jobs + history", r"PS1='\l:\j:\!:\# \$ '", [":0:1:1 "],
     [r"\l", r"\j", r"\!", r"\#"]),
    ("full hostname", r"PS1='\H \$ '", [], [r"\H"]),
    # ── raw ANSI in all its spellings ──────────────────────────────────
    ("\\e color", r"PS1='\e[32mG\e[0m\$ '", ["\x1b[32mG"], [r"\e["]),
    ("\\033 octal color", r"PS1='\033[31mR\033[0m\$ '", ["\x1b[31mR"],
     [r"\033"]),
    ("256-color", r"PS1='\e[38;5;208mO\e[0m\$ '", ["\x1b[38;5;208m"], []),
    ("truecolor RGB", r"PS1='\e[38;2;122;162;247mT\e[0m\$ '",
     ["\x1b[38;2;122;162;247m"], []),
    ("octal literal \\101", r"PS1='\101\102\103 \$ '", ["ABC"], [r"\101"]),
    ("bell + backslash", r"PS1='\a\\\\ \$ '", ["\\"], []),
    # ── \[ \] width markers, well- and ill-formed ──────────────────────
    ("width markers strip", r"PS1='\[\e[1m\]B\[\e[0m\]\$ '", ["B"],
     [r"\[", r"\]"]),
    ("UNCLOSED \\[ does not hang", r"PS1='\[\e[32m\u\$ '", [], []),
    ("stray \\] alone", r"PS1='\]ok\$ '", ["ok"], []),
    # ── $ expansions, live per render ──────────────────────────────────
    ("status via $?", r"PS1='[$?]\$ '", ["[0]"], []),
    ("braced ${?}", r"PS1='[${?}]\$ '", ["[0]"], []),
    ("plain variable", "export TAG=lively\nPS1='${TAG}\\$ '", ["lively"],
     ["TAG}"]),
    ("var with default", r"PS1='${UNSET_XYZ:-fallback}\$ '", ["fallback"],
     []),
    ("var :+ operator", "export ON=1\nPS1='${ON:+armed}\\$ '", ["armed"],
     []),
    ("arithmetic", r"PS1='<$((40 + 3))>\$ '", ["<43>"], []),
    ("dollar at end stays", r"PS1='cost$'", ["cost$"], []),
    ("cmdsub is NOT executed (by design)", r"PS1='\w $(pwd) \$ '",
     ["$(pwd)"], []),
    ("git-prompt style cmdsub survives as text",
     r"PS1='\w$(__git_ps1 \" (%s)\")\$ '", ["__git_ps1"], []),
    # ── PS1 is BILINGUAL: both escape languages render, and what keeps a
    # legacy percent safe is the mixed reader's own rules -- an unknown or
    # malformed `%` sequence stays literal (strict zsh would consume it),
    # and $-expansion / \D{...} spans are copied through verbatim. ──────
    ("literal percent survives", r"PS1='100% \$ '", ["100% "], []),
    ("csh-style %> survives", r"PS1='%> '", ["%> "], []),
    ("unknown escapes survive", r"PS1='%q %Y \$ '", ["%q %Y "], []),
    ("PS1 speaks %n@%m with no mode", r"PS1='<<%n>>@%m \$ '",
     ["<<", ">>@"], ["%n", "%m"]),
    ("PS1 speaks %F color with no mode", r"PS1='%F{green}ok%f\$ '",
     ["\x1b[32mok"], ["%F"]),
    ("PS1 speaks %(?..) with no mode", r"PS1='%(?.OK.NO) \$ '", ["OK "],
     ["%("]),
    ("strftime percents inside $() untouched",
     r"PS1='$(date +%Y-%m-%d) \$ '", ["$(date +%Y-%m-%d)"], []),
    ("strftime percents inside \\D{} untouched", r"PS1='[\D{%M}] \$ '",
     [], [r"\D", "%M"]),
    # ── shapes users actually build ────────────────────────────────────
    ("two-line boxed",
     r"PS1='\[\e[38;2;90;96;106m\]╭─\[\e[0m\] \u \w\n╰─ ❯ '",
     ["╭─", "╰─ ❯"], [r"\u"]),
    ("powerline segments",
     r"PS1='\[\e[1;38;2;20;20;20;48;2;152;195;121m\] ➜ \[\e[0m\] \W \$ '",
     ["➜"], [r"\["]),
    ("emoji prompt", r"PS1='🔥 \W \$ '", ["🔥"], []),
    ("hellish extensions kept", r"PS1='\g\S\p\J\U \$ '", [],
     [r"\g", r"\J"]),
    ("unknown escape passes through like bash", r"PS1='\q\$ '", [r"\q"],
     []),
    ("lone trailing backslash", "PS1='oops\\\\'", [], []),
    # An 80-column pty horizontally scrolls a prompt wider than the screen
    # and readline marks it with a leading '<' -- bash renders this the
    # same way, so only a screenful of the N's is ever visible at once.
    ("very long prompt (1 KB)", "PS1='" + "N" * 1024 + r"\$ '",
     ["N" * 40], []),
    ("tabs and CR kept literal", "PS1='a\tb\\$ '", [], []),
    # An unconfigured prompt is zsh's own default: "hostname% ".
    ("empty PS1 falls back to the zsh default", "PS1=''", ["% "], []),
    ("unset PS1 uses the zsh default", "unset PS1", ["% "], []),
    # ── the zsh PROMPT language ────────────────────────────────────────
    ("zsh identity", "PROMPT='<<%n>>@%m %# '", ["<<", ">>@"],
     ["%n", "%m"]),
    ("zsh cwd + status", "PROMPT='%~ [%?] %# '", ["~ [0]"], ["%~"]),
    ("zsh named colors", "PROMPT='%F{green}g%f%K{red}k%k %# '",
     ["\x1b["], ["%F", "%K"]),
    ("zsh 256 color", "PROMPT='%F{81}c%f %# '", ["\x1b["], ["%F{81}"]),
    ("zsh bold + jobs", "PROMPT='%B%~%b %j %# '", [], ["%B", "%j"]),
    ("zsh literal %%", "PROMPT='cpu 100%% %# '", ["cpu 100% "], ["%%"]),
    ("zsh full host + full cwd", "PROMPT='%M %d %# '", [], ["%M"]),
    ("zsh %N sourced-file name", "PROMPT='%N %# '", [], []),
    ("zsh malformed %F{ unclosed", "PROMPT='%F{unclosed x '", [], []),
    ("zsh unknown escape", "PROMPT='%Q %# '", [], []),
    ("zsh trailing lone percent", "PROMPT='end%'", [], []),
    ("mixed dialects in PROMPT", r"PROMPT='%n \w [$?] %# '", ["[0]"],
     ["%n", r"\w"]),
    # ── both variables at once ─────────────────────────────────────────
    ("PROMPT wins over PS1",
     "PS1='BASH-SIDE \\$ '\nPROMPT='ZSH-SIDE %# '", ["ZSH-SIDE"],
     ["BASH-SIDE"]),
    ("empty PROMPT falls back to PS1",
     "PROMPT=''\nPS1='LEGACY-\\u \\$ '", ["LEGACY-"], []),
    # With the dialect ARMED, PS1 becomes the zsh parameter -- in zsh, PS1
    # and PROMPT are the same variable. The bit decides, never the text:
    # every literal-% case above runs with the mode off and stays safe.
    ("zsh mode: PS1 speaks the % language",
     "set -o zsh\nPS1='<<%n>>%~ %# '", ["<<", ">>~ % "], ["%n", "%~"]),
    ("zsh mode: PROMPT still wins over PS1",
     "set -o zsh\nPS1='PS-SIDE %# '\nPROMPT='PR-SIDE %# '", ["PR-SIDE"],
     ["PS-SIDE"]),
    ("zsh conditional + truncation in PROMPT",
     "PROMPT='%(?.ok.no) %8<..<%~ %# '", ["ok "], []),
]


def one(name, rc, expects, rejects):
    text = run_case(rc)
    probs = []
    if "E2E-2026" not in text:
        probs.append("the session no longer executes commands")
    for s in GLOBAL_REJECTS + rejects:
        if s in text:
            probs.append("output contains %r" % s)
    for s in expects:
        if s not in text:
            probs.append("expected %r in the output" % s)
    check(name, not probs,
          "; ".join(probs) + "\n--- rc ---\n" + rc + "\n--- saw ---\n"
          + text[-500:] if probs else "")


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    print("\n\033[1;36m▸\033[0m \033[1m%d rc files, one real pty session "
          "each\033[0m (%s)" % (len(CASES), SHELL))
    threads = []
    sem = threading.Semaphore(8)

    def worker(c):
        with sem:
            one(*c)

    for c in CASES:
        t = threading.Thread(target=worker, args=(c,))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

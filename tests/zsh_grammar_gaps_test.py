#!/usr/bin/env python3
"""The grammar constructs that stop zsh-autosuggestions and zsh-syntax-
highlighting, measured rather than assumed. Issue #77.

WHY THIS EXISTS. #77 records both plugins as blocked by ZLE -- they want
`region_highlight`, which readline has no model for. That is true of what
they DO, and it is not what stops them from LOADING. Both define 0 functions
today because the PARSER rejects them, and which constructs do that had
never been measured. This file is that measurement, kept runnable so it
cannot rot into folklore.

HOW IT WAS FOUND. Each plugin was fed to hellish one line-prefix at a time
until a syntax error appeared, skipping the errors a truncated prefix causes
by itself (a cut inside a function body reports an unexpected EOF, which is
the cut's fault). Each blocker was then neutralised textually and the scan
re-run, so the list is ordered and complete rather than just the first wall.

WHAT IT WOULD BUY, measured the same way -- with the blockers rewritten by
hand, both plugins parse to the end and start defining things:

    zsh-autosuggestions       0 -> 29 functions   (needs 1 and 2)
    zsh-syntax-highlighting   0 -> 11 functions   (needs 2, 3 and 4)

So ZLE is not the first thing in the way for either of them. The parser is.

THE ORACLE. Every "zsh accepts this" below was run against a real zsh 5.9
(`zsh 5.9 (x86_64-ubuntu-linux-gnu)`), not reasoned out. Two results are
worth keeping precisely because they are counter-intuitive:

  * `;;&` is a BASH-ism. zsh rejects it too, so hellish already agrees with
    zsh and there is nothing to do.
  * `&>` already works here. It appeared in a failing line only because a
    DIFFERENT construct on that line was the real cause -- neutralising the
    `&>` moved the error not one character.

WHAT THIS ASSERTS. hellish's CURRENT answer for each construct. A gap that
starts working is a FAILURE here until the table is updated -- the same rule
tests/plugin_corpus_test.py uses, and for the same reason: a survey nobody
updates becomes a survey nobody believes. Implementing any of these is
deliberately NOT done here; #77's own note is that the parser work is in
flight elsewhere.

Usage: python3 zsh_grammar_gaps_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

# name, snippet, zsh-5.9 verdict, hellish verdict TODAY, what it blocks
CASES = [
    ("case-pattern alternation (a|b)",
     'case foo_bound_x in foo_(bound|orig)_*) echo M;; esac',
     "accept", "reject",
     "zsh-autosuggestions:156 -- `user:_zsh_autosuggest_(bound|orig)_*)`"),

    ("anonymous function () { }",
     '() { echo A; }',
     "accept", "reject",
     "zsh-autosuggestions:460 and zsh-syntax-highlighting:259"),

    ("`;&` case fall-through",
     'case a in a) echo 1 ;& b) echo 2 ;; esac',
     "accept", "reject",
     "zsh-syntax-highlighting:144 -- being added in src/lexer/, see #82"),

    ("empty compound body (only a comment)",
     'if true; then\n# nothing\nelif false; then echo B\nfi',
     "accept", "reject",
     "zsh-syntax-highlighting:522 -- an `if ... then` whose body is a "
     "comment. bash rejects this too; it is zsh permissiveness."),

    ("empty function body",
     'f() { # nothing\n}',
     "accept", "reject",
     "same class as the above"),

    # The two that are NOT gaps. Kept so a regression is caught and so the
    # next reader does not re-investigate them.
    ("`;;&` continue-testing",
     'case a in a) echo 1 ;;& a*) echo 2 ;; esac',
     "reject", "reject",
     "a bash-ism zsh also rejects -- hellish already agrees with zsh"),

    ("`&>` redirect",
     'true &> /dev/null',
     "accept", "accept",
     "already supported; it was a red herring in the failing line"),
]


def check(name, ok, detail=""):
    mark = "\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m"
    print("  %s %s" % (mark, name), flush=True)
    if not ok:
        if detail:
            print("       %s" % str(detail).replace("\n", "\n       "))
        FAILS.append(name)


def verdict(snippet):
    """What hellish does with this construct, in the zsh dialect."""
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_ANIM="1",
               HELLISH_NO_UPDATE_CHECK="1")
    r = subprocess.run([SHELL, "-c", "setopt zsh\n" + snippet],
                       capture_output=True, text=True, env=env, timeout=30)
    return "reject" if "syntax error" in (r.stderr + r.stdout) else "accept"


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    print("\n\033[1;36m▸\033[0m \033[1mzsh grammar constructs, vs hellish "
          "today\033[0m")
    for name, snippet, zsh, want, blocks in CASES:
        got = verdict(snippet)
        if got == want:
            note = "gap" if zsh != got else "agrees with zsh"
            check("%-38s %s" % (name, note), True)
        else:
            check("%-38s" % name, False,
                  "hellish now %sS this; zsh %sS it.\n"
                  "  Blocks: %s\n"
                  "  If this was implemented on purpose, update CASES here "
                  "and re-check\n  whether the plugin now loads -- that is "
                  "the point of the table."
                  % (got.upper(), zsh.upper(), blocks))

    gaps = [c for c in CASES if c[2] != c[3]]
    print("\n  %d of %d constructs are gaps; closing them is what makes "
          "zsh-autosuggestions\n  and zsh-syntax-highlighting parse at all "
          "(29 and 11 functions respectively)."
          % (len(gaps), len(CASES)))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

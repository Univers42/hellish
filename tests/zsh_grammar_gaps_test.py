#!/usr/bin/env python3
"""The zsh grammar constructs that decide whether two real plugins load.

WHY THIS EXISTS. #77 records zsh-autosuggestions and zsh-syntax-highlighting
as blocked by ZLE -- they want `region_highlight`, which readline has no
model for. That is true of what they DO, and it was never what stopped them
from LOADING. Both defined 0 functions because the PARSER rejected them on
line 1, and which constructs did that had not been measured. This file is
that measurement, kept runnable so it cannot rot back into folklore.

HOW THE LIST WAS FOUND. Each plugin was fed to hellish one line-prefix at a
time until a syntax error appeared, skipping the errors a truncated prefix
causes by itself (a cut inside a function body reports an unexpected EOF,
which is the cut's fault). Each blocker was then neutralised textually and
the scan re-run, so the list is ordered and complete rather than just the
first wall.

THE ORACLE. Every "zsh accepts this" below was run against a real zsh 5.9
(`zsh 5.9 (x86_64-ubuntu-linux-gnu)`), not reasoned out. Two results are
worth keeping precisely because they are counter-intuitive:

  * `;;&` is a BASH-ism. zsh rejects it -- hellish accepts it, because
    hellish is bash-first and the golden suite pins bash's reading. That is
    a deliberate divergence from zsh, recorded here rather than argued about
    twice.
  * `&>` already worked. It appeared in a failing line only because a
    DIFFERENT construct on that line was the real cause; neutralising the
    `&>` moved the error not one character.

HOW THE DIALECT IS ARMED, and why it is not `setopt zsh`.

An earlier version of this file prefixed each snippet with `setopt zsh` and a
newline and ran it through `-c`. That works for a one-line snippet and
silently does NOT work for a multi-line one: a compound command spanning
lines is gathered and parsed BEFORE the previous line has executed, so
`setopt zsh` had not run yet and the construct was measured in the bash
dialect. Two rows of this table were reporting on the wrong shell.

Everything here now goes through a `.zsh` FILE, which is how a plugin
actually arrives (src/core/zsh_mode.c: the extension arms the dialect for
the whole file). The `.sh` control below runs the identical bytes through
the bash dialect and requires them to be REFUSED -- without it, "zsh mode
accepts this" would also pass on a shell that had simply gone permissive
everywhere, which is the failure the golden suite exists to prevent.

Usage: python3 zsh_grammar_gaps_test.py [/path/to/hellish]
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
CORPUS = os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus"))
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
FAILS = []

# name, snippet, zsh 5.9, hellish-in-zsh TODAY, note
#
# "accept" here means "parses". Everything that parses is additionally
# asserted on its BEHAVIOUR below, because a construct that parses and then
# does the wrong thing is worse than one that is refused.
CASES = [
    ("case-pattern alternation (a|b)",
     'case foo_bound_x in foo_(bound|orig)_*) echo M;; esac',
     "accept", "accept",
     "zsh-autosuggestions:156 -- `user:_zsh_autosuggest_(bound|orig)_*)`"),

    ("anonymous function () { }",
     '() { echo A; }',
     "accept", "accept",
     "zsh-autosuggestions:460, zsh-syntax-highlighting:259 and :416"),

    ("`;&` case fall-through",
     'case a in a) echo 1 ;& b) echo 2 ;; esac',
     "accept", "accept",
     "zsh-syntax-highlighting:144"),

    ("empty compound body (only a comment)",
     'if true; then\n# nothing\nelif false; then echo B\nfi',
     "accept", "accept",
     "zsh-syntax-highlighting:522 -- an `if ... then` whose body is only a "
     "comment. bash refuses this and still must; it is zsh permissiveness."),

    ("empty function body",
     'f() { # nothing\n}',
     "accept", "accept",
     "same class as the above"),

    # Not gaps. Kept so a regression is caught and so the next reader does
    # not re-investigate them.
    ("`;;&` continue-testing",
     'case a in a) echo 1 ;;& a*) echo 2 ;; esac',
     "reject", "accept",
     "a bash-ism; hellish is bash-first and keeps bash's reading"),

    ("`&>` redirect",
     'true &> /dev/null',
     "accept", "accept",
     "already supported; it was a red herring in the failing line"),
]

# The constructs above, asserted on what they DO. Parsing is necessary and
# nowhere near sufficient: a `(a|b)` that parses and then matches literally,
# or an anonymous function whose `return` returns from the SOURCING file,
# would both pass the table and break the plugin.
BEHAVIOUR = [
    ('case foo_bound_x in foo_(bound|orig)_*) echo M;; *) echo N;; esac',
     "M"),
    ('case foo_orig_y in foo_(bound|orig)_*) echo M;; *) echo N;; esac',
     "M"),
    ('case foo_zzz_y in foo_(bound|orig)_*) echo M;; *) echo N;; esac',
     "N"),
    ('[[ foo_bound_x == foo_(bound|orig)_* ]] && echo M || echo N', "M"),
    # The same pattern as a FILENAME glob. It is the same matcher, and what
    # made it not be was a fast path that claimed the word as a plain
    # literal and never called the glob walk -- so `case` said "match" and
    # `echo` printed the pattern straight back. One pattern, two answers,
    # depending only on where it was written.
    ('mkdir -p zg/alpha zg/beta; echo zg/(alpha|beta)',
     "zg/alpha zg/beta"),
    ('mkdir -p zg/alpha zg/beta; echo zg/al(pha|PHA)', "zg/alpha"),
    # An anonymous function is a real call frame, not a brace group.
    ('() { local v=IN; echo "$v"; }\necho "out=[${v-unset}]"',
     "IN\nout=[unset]"),
    ('() { echo A; return; echo NOT_REACHED; }\necho B', "A\nB"),
    ('() { echo X; return 7; }\necho "rc=$?"', "X\nrc=7"),
    ('outer() { () { echo in; }; echo out; }\nouter', "in\nout"),
    # ...and it can still set a global, which is the only reason the two
    # plugins use it at load time at all.
    ('() { typeset -g G=set; }\necho "G=$G"', "G=set"),
    # An empty body is empty -- not "fall into the next branch".
    ('if false; then\n# nothing\nelse\necho ELSE\nfi', "ELSE"),
    ('if true; then\n# nothing\nelse\necho ELSE\nfi', ""),
    ('f() { # nothing\n}\nf; echo "rc=$?"', "rc=0"),
]

# What must STILL be refused in the bash dialect. The empty-body rows are
# the ones that matter: bash calls them a syntax error, the golden suite
# pins that, and a permissiveness that leaked out of the dialect gate would
# be invisible here without an explicit control.
BASH_MUST_REFUSE = [
    'if true; then\n# nothing\nfi',
    'f() { # nothing\n}',
    'case foo_bound_x in foo_(bound|orig)_*) echo M;; esac',
    '() { echo A; }',
]


def check(name, ok, detail=""):
    mark = "\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m"
    print("  %s %s" % (mark, name), flush=True)
    if not ok:
        if detail:
            print("       %s" % str(detail).replace("\n", "\n       "))
        FAILS.append(name)


def run_file(snippet, ext, cwd=None):
    """Run a snippet as a FILE with the given extension -- `.zsh` arms the
    dialect for the whole file, `.sh` does not."""
    fd, path = tempfile.mkstemp(suffix=ext)
    with os.fdopen(fd, "w") as f:
        f.write(snippet + "\n")
    try:
        p = subprocess.run([SHELL, path], capture_output=True, text=True,
                           timeout=60, env=ENV, cwd=cwd)
        return p.returncode, p.stdout, p.stderr
    finally:
        os.unlink(path)


def verdict(snippet, ext=".zsh"):
    _, out, err = run_file(snippet, ext)
    if "syntax error" in (err + out):
        return "reject"
    return "accept"


def table_cases():
    print("\n\033[1;36m>\033[0m \033[1mthe constructs, vs hellish "
          "today\033[0m")
    for name, snippet, zsh, want, note in CASES:
        got = verdict(snippet)
        if got == want:
            state = "agrees with zsh"
            if zsh != got:
                state = "DIVERGES from zsh, on purpose"
            check("%-38s %s" % (name, state), True)
        else:
            check("%-38s" % name, False,
                  "hellish now %sS this; zsh %sS it; this table says %sS.\n"
                  "  Note: %s\n"
                  "  If this changed on purpose, update CASES here AND "
                  "re-check whether\n  the plugin now loads -- that is the "
                  "point of the table."
                  % (got.upper(), zsh.upper(), want.upper(), note))


def behaviour_cases():
    """Parsing is not the claim. This is."""
    print("\n\033[1;36m>\033[0m \033[1mwhat they actually do\033[0m")
    tmp = tempfile.mkdtemp(prefix="zsh_gram_")
    try:
        for snippet, expect in BEHAVIOUR:
            _, out, err = run_file(snippet, ".zsh", cwd=tmp)
            got = out.strip()
            check("%-46s" % snippet.split("\n")[0][:46],
                  got == expect and "syntax error" not in err,
                  "want %r\n  got  %r  (err=%r)" % (expect, got, err[:160]))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def bash_dialect_cases():
    """The control. Every construct above is zsh-only, so the identical
    bytes in a `.sh` file must still be refused -- otherwise the dialect
    gate has leaked and the golden suite is what breaks next."""
    print("\n\033[1;36m>\033[0m \033[1mand the bash dialect still refuses "
          "them\033[0m")
    for snippet in BASH_MUST_REFUSE:
        check("%-46s" % snippet.split("\n")[0][:46],
              verdict(snippet, ".sh") == "reject",
              "the .sh dialect ACCEPTED a zsh-only construct; the gate in "
              "zsh_mode.c has leaked")


def plugin_cases():
    """The acceptance test: the two plugins #77 named. Both defined 0
    functions before this work, because the parser stopped on line 1.

    The floor is what each gets to today, so a regression shows up as a
    number going DOWN. Neither is asserted to WORK: autosuggestions wants
    `is-at-least` from zsh's own function distribution, and
    syntax-highlighting stops at its own check for the `zsh/zleparameter`
    module. Both of those are honest failures, and the second one is still
    #77 item 4."""
    print("\n\033[1;36m>\033[0m \033[1mthe two plugins #77 named\033[0m")
    for name, floor in (("zsh-autosuggestions", 29),
                        ("zsh-syntax-highlighting", 11)):
        path = os.path.join(CORPUS, name + ".zsh")
        if not os.path.exists(path):
            print("  skip %s (not cached)" % name)
            continue
        p = subprocess.run(
            [SHELL, "-c", "source %s 2>/dev/null; declare -F | wc -l" % path],
            capture_output=True, text=True, timeout=90, env=ENV)
        try:
            n = int(p.stdout.strip().split()[-1])
        except (ValueError, IndexError):
            n = -1
        check("%-38s defines >= %d (got %d)" % (name, floor, n), n >= floor,
              "it defined %d functions; it used to define 0 because the "
              "parser stopped on line 1. A DROP means one of the constructs "
              "above regressed." % n)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        return 2
    table_cases()
    behaviour_cases()
    bash_dialect_cases()
    plugin_cases()
    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

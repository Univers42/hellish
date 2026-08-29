#!/usr/bin/env python3
"""Regression test: `function name { }` -- #71 item 5.1.

Three spellings of a function definition; hellish accepted only the first:

    name () { }             POSIX      -- worked
    function name { }       ksh/bash   -- syntax error
    function name () { }    bash/zsh   -- syntax error

It is the single construct that takes the real zsh plugin corpus from 4
plugins loading to 5, and most copy-pasted snippets on the internet use it.

TWO things had to change, and the second is the interesting one:

  * the parser (is_function_def / parse_function_def) looked for exactly
    WORD ( ) and popped exactly three tokens;
  * the LEXER left `{` as TT_WORD. `{` is only promoted to TT_LBRACE while
    the reclassifier believes it is at a command position, and after
    `function name` it does not -- two plain WORDs in a row look like a
    command and its argument. So even with the parser fixed, the brace was
    still a word and the error just moved to `}`. TT_COPROC already carries
    a latch for exactly this ("the next word is still a command position");
    `function` now arms the same one.

`function` is NOT promoted to a reserved word here, and that is a DELIBERATE
divergence from bash rather than an oversight.

bash reserves it: `function() { ...; }` and a bare `function` are both syntax
errors there. hellish matches the word only in the shape "function <name>",
so those two keep working. The divergence is one-directional -- every script
bash accepts, hellish accepts identically -- and it buys a smaller blast
radius: promoting a common English word to a keyword would break any script
using it as a command or a variable, which is a worse bug than the one being
fixed. The two cases are asserted below AS divergences, so if someone later
makes it a true reserved word, that is a deliberate change and not a silent
one.

Usage: python3 function_keyword_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(sh, script):
    p = subprocess.run([sh, "-c", script], capture_output=True, text=True,
                       env=ENV, timeout=60)
    return p.stdout, p.returncode


def same_as_bash(name, script):
    got, grc = run(SHELL, script)
    if not os.path.exists(ORACLE):
        return check(name + " (no oracle, skipped)", True)
    want, wrc = run(ORACLE, script)
    check(name, got == want and grc == wrc,
          "hellish=%r(%d) bash=%r(%d)" % (got, grc, want, wrc))


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    # --- the three spellings -----------------------------------------
    same_as_bash("name () { }", 'h() { echo C; }; h')
    same_as_bash("function name { }", 'function f { echo A; }; f')
    same_as_bash("function name () { }", 'function g () { echo B; }; g')

    # --- `function` must NOT become a reserved word ------------------
    same_as_bash("`echo function` still prints it", 'echo function')
    same_as_bash("a variable may be called function", 'f=function; echo "$f"')
    # DIVERGENCE (deliberate): bash rejects both of these because it
    # reserves `function`. hellish accepts them, which is strictly more
    # permissive -- nothing bash accepts changes meaning.
    out, rc = run(SHELL, 'function() { echo D; }; function')
    check("DIVERGENCE: a function may be named `function`",
          out.strip() == "D" and rc == 0,
          "hellish rejected it too -- if that is now intended, update this "
          "test; bash also rejects it")
    same_as_bash("function as an argument", 'printf "[%s]" function; echo')

    # --- shapes the parser must not mistake for a definition ---------
    out, rc = run(SHELL, 'function 2>/dev/null; echo "rc=$?"')
    check("DIVERGENCE: bare `function` runs as a command (bash: syntax error)",
          "rc=127" in out, "got %r" % out.strip())
    same_as_bash("assignment prefix is not a definition",
                 'x=1 true; echo "rc=$?"')

    # --- body forms ---------------------------------------------------
    same_as_bash("multi-line body",
                 'function m {\n  local x=1\n  echo "m$x"\n}\nm')
    same_as_bash("body with a nested compound",
                 'function n { if true; then echo N; fi; }; n')
    same_as_bash("redefinition wins",
                 'function r { echo 1; }; function r { echo 2; }; r')
    same_as_bash("declare -F sees it",
                 'function s { :; }; declare -F s')

    # --- churn, per the t_scope_save lesson ---------------------------
    out, rc = run(SHELL,
                  'i=0; while [ $i -lt 300 ]; do '
                  'eval "function k$i { echo -n \'\'; }"; k$i; '
                  'unset -f k$i; i=$((i+1)); done; echo survived')
    check("300 define/call/unset rounds do not crash",
          rc == 0 and out.strip() == "survived",
          "rc=%d out=%r" % (rc, out.strip()))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

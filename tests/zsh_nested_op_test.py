#!/usr/bin/env python3
"""zsh `aliases`, flags on subscripts, and a subscript + operator after a
nested expansion -- the three expansion gaps behind issue #137.

oh-my-zsh's sudo widget (on ESC ESC) asks "what command does this alias
run" as `"${${(Az)aliases[$cmd]}[1]:-$cmd}"`. On hellish that was a bad
substitution three ways over:

  * `aliases` read empty -- its WRITES reached the alias table (#114),
    its reads fell through to a plain, empty variable;
  * a flag on a subscripted name, `${(z)aliases[ll]}` or `${(U)e[2]}`,
    reached the scalar engine, which has no subscript;
  * a subscript WITH an operator after a nested expansion, `${${..}[1]:-x}`,
    did the same -- and when it did get through, the nested value had been
    joined first, so `[1]` took a character instead of a word.

Every expectation below is what zsh 5.8 printed (zsh -f, with `unalias -a`
first: zsh -f still predefines run-help and which-command). They are
asserted here so the check runs where no zsh is installed -- CI's default
-- and when a zsh oracle IS found (ZSH_ORACLE, ~/zsh-5.9, or PATH), each
case is also run under it, so an expectation cannot drift from zsh.

Only double-quoted forms and forms whose result is one word are pinned:
hellish's zsh dialect still field-splits unquoted expansions (zsh does not
without SH_WORD_SPLIT), which is a separate, dialect-wide difference.

Usage: python3 zsh_nested_op_test.py [/path/to/hellish]
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

PRE = ("unalias -a; alias ll='ls -l' gs='git status'; cmd=ll; c2=vim; "
       "e=(p 'q r' s); x='a b c'; ")

# label, script (after PRE), what zsh 5.8 printed
CASES = [
    ("aliases/read", r"""printf '[%s]' "${aliases[ll]}" """, "[ls -l]"),
    ("aliases/read-unset", r"""printf '[%s]' "${aliases[nope]}" """, "[]"),
    ("aliases/count", r"""printf '[%s]' ${#aliases}""", "[2]"),
    ("aliases/keys", r"""printf '[%s]' ${(ok)aliases}""", "[gs][ll]"),
    ("aliases/values", r"""printf '[%s]' "${(@ov)aliases}" """,
     "[git status][ls -l]"),
    ("aliases/defined-since", r"""alias zz=echo; printf '[%s]' "${aliases[zz]}" """,
     "[echo]"),
    ("aliases/gone-after-unalias",
     r"""unalias ll; printf '[%s]' "${aliases[ll]:-gone}" """, "[gone]"),
    ("widget/alias", r"""printf '[%s]' "${${(Az)aliases[$cmd]}[1]:-$cmd}" """,
     "[ls]"),
    ("widget/not-an-alias",
     r"""printf '[%s]' "${${(Az)aliases[$c2]}[1]:-$c2}" """, "[vim]"),
    ("widget/buffer-first-word",
     r"""BUFFER='sudo -e f'; printf '[%s]' "${${(Az)BUFFER}[1]}" """,
     "[sudo]"),
    ("flag/on-array-element", r"""printf '[%s]' "${(U)e[2]}" """, "[Q R]"),
    ("flag/on-alias-element", r"""printf '[%s]' "${(U)aliases[gs]}" """,
     "[GIT STATUS]"),
    ("nested/word-then-default", r"""printf '[%s]' "${${(z)x}[2]:-d}" """,
     "[b]"),
    ("nested/past-the-end", r"""printf '[%s]' "${${(z)x}[5]:-dflt}" """,
     "[dflt]"),
    ("nested/alternate", r"""printf '[%s]' "${${(z)x}[2]:+set}" """,
     "[set]"),
    ("nested/scalar-char", r"""printf '[%s]' "${${(U)x}[1]:-d}" """, "[A]"),
    ("nested/quoted-array-joins",
     r"""printf '[%s]' "${${(U)e}[2]:-d}" """, "[ ]"),
    ("nested/unflagged", r"""printf '[%s]' "${${x}[1]:-d}" """, "[a]"),
    ("nested/trim-after-subscript",
     r"""printf '[%s]' "${${(z)x}[3]#c}" """, "[]"),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def find_zsh():
    env = os.environ.get("ZSH_ORACLE")
    if env and os.path.exists(env):
        return env
    home = os.path.expanduser("~/zsh-5.9/bin/zsh")
    for c in (home, shutil.which("zsh"), "/usr/bin/zsh", "/bin/zsh"):
        if c and os.path.exists(c):
            return c
    return None


def run(argv, script):
    p = subprocess.run(argv + [script], capture_output=True, timeout=10,
                       env=dict(os.environ, HELLISH_NO_BANNER="1",
                                HELLISH_NO_UPDATE_CHECK="1"))
    return p.stdout.decode(errors="replace"), p.stderr.decode(errors="replace")


def main():
    zsh = find_zsh()
    for name, script, want in CASES:
        out, err = run([SHELL, "-c"], "set -o zsh\n" + PRE + script)
        check(name, out == want and "bad substitution" not in err,
              "got %r want %r err=%r" % (out, want, err.strip()[:120]))
        if zsh:
            zout, _ = run([zsh, "-f", "-c"], PRE + script)
            check("oracle/" + name, zout == want,
                  "zsh printed %r, the pinned expectation is %r" % (zout, want))
    if not zsh:
        print("SKIP oracle/*  (no zsh; set ZSH_ORACLE to cross-check)")
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

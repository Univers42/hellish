#!/usr/bin/env python3
"""Regression test: `shopt -o` addresses the set -o options -- issue #51.

The report is a `make my_shell` login on Ubuntu 24 that printed errors.
Two of them come from stock dotfiles probing shopt, and both are this
builtin's fault.

  1. ~/.bashrc (Ubuntu's skel copy, pulled in by ~/.profile because hellish
     advertises BASH_VERSION) runs

         if ! shopt -oq posix; then ... fi

     to decide whether to load completions. builtin_shopt() dispatched
     `-o` to list_set_options() and threw away the NAMES, the -q and the
     -p: the login printed the entire 27-line option table, and returned 0
     regardless of the setting, so the branch was decided by a coin flip.

  2. /etc/profile.d/bash_completion.sh runs `shopt -q progcomp`. hellish
     did not know the name, so every login opened with

         hellish: shopt: progcomp: invalid shell option name

     progcomp landed as a known option that was OFF, which was the
     truthful answer while hellish had no `complete` builtin.

     It has one now, and TAB consults its specs (#72 phase 4), so the
     truthful answer changed with the shell: progcomp is ON, as it is in
     bash. Turning it on has a consequence worth knowing about -- on a host
     with bash-completion installed, bash_completion.sh will now try to
     source its 3800-line framework, which hellish cannot yet parse (the
     lexer runs over a whole sourced file before `shopt -s extglob` on line
     47 has executed). That is recorded as an `unsupported` row in
     tests/plugin_corpus_test.py rather than papered over by leaving the
     option lying about itself.

Everything asserted below was read off bash 5.x first; each case carries
the bash command that produced it.

Usage: python3 shopt_setopt_test.py /path/to/hellish
"""
import os
import subprocess
import sys

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "build/bin/hellish")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(script):
    """Run one -c script; return (stdout, stderr, status)."""
    env = dict(os.environ)
    env.update({"HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
                "ASAN_OPTIONS": "detect_leaks=0"})
    p = subprocess.run([SHELL, "-c", script], capture_output=True, text=True,
                       env=env, timeout=30)
    return p.stdout, p.stderr, p.returncode


def case(name, script, out=None, err="", rc=None, out_absent=None):
    o, e, r = run(script)
    if rc is not None:
        check("%s: status %d" % (name, rc), r == rc, "got %d" % r)
    if out is not None:
        check("%s: stdout" % name, o == out, "got %r want %r" % (o, out))
    if out_absent is not None:
        check("%s: stdout free of %r" % (name, out_absent),
              out_absent not in o, "got %r" % o[:200])
    if err is not None:
        check("%s: stderr" % name, (e == "") if err == "" else (err in e),
              "got %r" % e[:200])


def main():
    # ── the two probes from the report ────────────────────────────────────
    # bash: `shopt -oq posix` -> no output, status 1 (posix is off).
    # The old code printed the whole table and returned 0.
    case("shopt -oq posix", "shopt -oq posix", out="", rc=1)
    # The table dump is the visible half of the bug: assert it explicitly,
    # because an empty stdout check alone would not say WHY it must be empty.
    case("shopt -oq posix does not dump the table", "shopt -oq posix",
         out_absent="braceexpand", rc=1)

    # bash: `shopt -q progcomp` -> silent, status 0, because bash HAS
    # programmable completion. So does hellish now, so the answer is the
    # same one for the same reason. What #51 needed either way is that it
    # does not ERROR.
    case("shopt -q progcomp is silent and on", "shopt -q progcomp",
         out="", err="", rc=0)
    # And it is a real switch, not a stored bit: unsetting it turns the
    # dispatch off (proved end to end in progcomp_test.py case 8).
    case("shopt -u progcomp then -q reports off",
         "shopt -u progcomp; shopt -q progcomp", out="", err="", rc=1)
    o, e, r = run("shopt -q progcomp; echo rc=$?")
    check("shopt -q progcomp prints no error at all",
          "invalid" not in e and "invalid" not in o,
          "stdout=%r stderr=%r" % (o[:120], e[:120]))

    # ── `shopt -o` with a name, every modifier ────────────────────────────
    # bash: `shopt -o posix` -> "posix          \toff", status 1.
    case("shopt -o posix", "shopt -o posix", out="posix          \toff\n",
         rc=1)
    # bash: `shopt -op posix` -> "set +o posix", status 1.
    case("shopt -op posix", "shopt -op posix", out="set +o posix\n", rc=1)
    # bash: status follows the SETTING for the query forms.
    case("shopt -oq xtrace, off", "shopt -oq xtrace", out="", rc=1)
    case("shopt -oq xtrace, on", "set -x 2>/dev/null; shopt -oq xtrace",
         rc=0, err=None)
    # bash: -s/-u through -o really move the set -o option.
    case("shopt -so noclobber sets it",
         "shopt -so noclobber; shopt -oq noclobber; echo rc=$?",
         out="rc=0\n", rc=0)
    case("shopt -uo noclobber clears it",
         "set -C; shopt -uo noclobber; shopt -oq noclobber; echo rc=$?",
         out="rc=1\n", rc=0)
    # bash: an unknown -o name is "invalid option name" (the -o form drops
    # the word "shell" that the plain form uses), status 1.
    case("shopt -o extglob rejects a non-set-o name", "shopt -o extglob",
         out="", err="invalid option name", rc=1)
    # bash: bare `shopt -o` still lists the whole set -o table, status 0.
    o, e, r = run("shopt -o")
    check("bare shopt -o still lists the table",
          "braceexpand" in o and "xtrace" in o and r == 0,
          "rc=%d out=%r" % (r, o[:160]))

    # ── the plain shopt forms must not have moved ─────────────────────────
    case("shopt -q histappend, off by default", "shopt -q histappend",
         out="", rc=1)
    case("shopt -s extglob then -q", "shopt -s extglob; shopt -q extglob",
         out="", rc=0)
    case("plain shopt name still prints on/off", "shopt -s dotglob; "
         "shopt dotglob", out="dotglob             \ton\n", rc=0)
    case("unknown plain name still errors like bash", "shopt notanoption",
         out="", err="invalid shell option name", rc=1)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

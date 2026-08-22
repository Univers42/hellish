#!/usr/bin/env python3
"""The `pretty` builtin: named presets over the behaviour knobs (issue #32).

Not a golden-suite category, because bash has no `pretty` builtin -- every
case would diff against "command not found". So the contract is asserted
directly here, and the file lands in tests/ where pty_suite.sh discovers
it with no wiring.

What is actually under test is the two properties the feature was asked
for, not the wording of its output:

  REPRODUCIBLE   `pretty -p` prints lines that, pasted into ~/.hellishrc,
                 reproduce the same configuration. So feeding its output
                 back in and asking again must give a byte-identical
                 answer -- a fixed point. That is what makes a config
                 portable between machines instead of remembered.

  ONE TRUTH      every pretty feature IS a shopt bit, not a copy of one.
                 If the two could disagree, the shell would have two
                 answers to "is multi-line recall on?" and users would hit
                 whichever one their tooling asked. So the test flips each
                 one through `pretty` and reads it back through `shopt`,
                 and vice versa.

The behaviour each feature selects is tested where that behaviour lives
(multiline-history in history_multiline_matrix.py, globs in the golden
suite). This file tests the CONFIG layer.

Usage: python3 pretty_modes_test.py /path/to/hellish
"""
import os
import subprocess
import sys

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []

ENV = dict(os.environ)
ENV.update({"HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0"})

# name -> the shopt option it must BE. Kept here deliberately: if someone
# repoints a pretty feature at a different bit, this table is the thing
# that has to change with it, in the same commit.
FEATURES = {
    "multiline-history": "lithist",
    "cd-spell": "cdspell",
    "auto-cd": "autocd",
    "resize-aware": "checkwinsize",
    "deep-glob": "globstar",
    "extended-glob": "extglob",
    "case-blind-glob": "nocaseglob",
    "hidden-glob": "dotglob",
}
MODES = ("plain", "friendly", "full")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def sh(script):
    """Run one script through the shell; return (stdout, stderr, status)."""
    p = subprocess.run([SHELL, "-c", script], capture_output=True,
                       env=ENV, timeout=60)
    return (p.stdout.decode(errors="replace"),
            p.stderr.decode(errors="replace"), p.returncode)


def as_script(pretty_p):
    """Turn `pretty -p` output into something runnable on one -c line.

    strip() first: the output ends in a newline, and joining on "; " without
    stripping yields a trailing empty command -- `a; b; ; pretty -p` -- which
    is a syntax error. That is a bug in this harness, not in the shell, and
    it is worth naming because it looked exactly like a round-trip failure.
    """
    return "; ".join(l for l in pretty_p.strip().split("\n") if l.strip())


def test_reproducible():
    for mode in MODES:
        first, _, st = sh("pretty mode %s; pretty -p" % mode)
        check("pretty mode %s succeeds" % mode, st == 0, "status %d" % st)
        # the paste-back property: replay its own output, ask again
        again, _, _ = sh("%s; pretty -p" % as_script(first))
        check("pretty -p round-trips for mode %s" % mode, first == again,
              "\n        first %r\n        again %r" % (first, again))
    # ... and it must round-trip from an arbitrary set too, not just presets
    script = "; ".join("pretty on " + f for f in list(FEATURES)[:3])
    first, _, _ = sh("pretty mode plain; %s; pretty -p" % script)
    again, _, _ = sh("pretty mode plain; %s; pretty -p" % as_script(first))
    check("pretty -p round-trips for a hand-picked set", first == again,
          "\n        first %r\n        again %r" % (first, again))


def test_one_source_of_truth():
    for feat, opt in FEATURES.items():
        out, _, _ = sh("pretty mode plain; pretty on %s; shopt %s"
                       % (feat, opt))
        check("pretty on %s sets shopt %s" % (feat, opt), "on" in out,
              "shopt said %r" % out.strip())
        out, _, _ = sh("pretty mode full; pretty off %s; shopt %s"
                       % (feat, opt))
        check("pretty off %s clears shopt %s" % (feat, opt),
              "off" in out, "shopt said %r" % out.strip())
        out, _, _ = sh("pretty mode plain; shopt -s %s; pretty -p" % opt)
        check("shopt -s %s shows up in pretty -p" % opt,
              ("pretty on %s" % feat) in out, "pretty -p said %r" % out)


def test_modes():
    out, _, _ = sh("pretty mode full; pretty mode plain; pretty -p")
    check("mode plain turns everything OFF, it does not merge",
          out.strip() == "pretty mode plain", "got %r" % out)
    out, _, _ = sh("pretty mode full; pretty -p")
    check("mode full enables every feature",
          all(("pretty on %s" % f) in out for f in FEATURES),
          "got %r" % out)
    out, _, _ = sh("pretty mode friendly; pretty -p")
    check("mode friendly includes multi-line recall (the #32 ask)",
          "pretty on multiline-history" in out, "got %r" % out)
    # a mode is a one-shot assignment, so an explicit toggle after it wins
    out, _, _ = sh("pretty mode friendly; pretty off multiline-history;"
                   " pretty -p")
    check("a toggle after a mode wins",
          "pretty on multiline-history" not in out, "got %r" % out)


def test_errors_and_defaults():
    for bad, what in (("pretty on nope", "unknown feature"),
                      ("pretty off nope", "unknown feature"),
                      ("pretty mode nope", "unknown mode"),
                      ("pretty bogus", "unknown subcommand"),
                      ("pretty on", "on with no name"),
                      ("pretty mode", "mode with no name")):
        _, err, st = sh(bad)
        check("`%s` is an error, not a silent no-op (%s)" % (bad, what),
              st == 2 and err.strip() != "",
              "status %d, stderr %r" % (st, err))
    out, _, st = sh("pretty")
    check("bare `pretty` reports and exits 0", st == 0, "status %d" % st)
    out, _, _ = sh("pretty mode plain; pretty")
    check("bare `pretty` with nothing on says so, rather than printing "
          "an empty screen", out.strip() != "", "got %r" % out)


def main():
    test_reproducible()
    test_one_source_of_truth()
    test_modes()
    test_errors_and_defaults()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

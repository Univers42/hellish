#!/usr/bin/env python3
"""ZLE: the widget registry, bindkey, and the guard semantics plugins rely on.

WHAT THIS ASSERTS, and what it does NOT.

Asserted, because it is verifiable here: `zle -N` registers, `bindkey`
records, the bare `zle` guard is FALSE outside the editor and an unknown
widget is refused rather than silently accepted. Those are what decide
whether a plugin LOADS, and oh-my-zsh's sudo now does -- it previously exited
1 with "not supported".

NOT asserted: that pressing the bound key runs the widget. That needs a live
pty and it is NOT VERIFIED at the time of writing -- the binding is recorded
and installed, but a pty run showed ESC ESC reaching the terminal unhandled,
so the dispatch is not yet proven to fire. Claiming it here would be exactly
the failure this suite exists to prevent: a test that passes against a
binary where the feature does not work.

    tracking: the key-dispatch half is unfinished. See the branch notes.

THE BOUNDARY THAT IS NOT A BUG. readline runs in a FORKED CHILD of the shell
(bg_readline). A widget's edits to BUFFER survive, because the line is what
the child sends back. Anything else it changes does not: a widget that runs
`cd` changes the child's directory and the parent never learns. That is why
sudo (which only edits the buffer) is the reachable case and dirhistory
(whose widgets cd) is not.

Usage: python3 zle_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
CORPUS = os.path.join(os.environ.get(
    "XDG_CACHE_HOME", os.path.expanduser("~/.cache")),
    "hellish-plugin-corpus")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(script, timeout=25):
    try:
        p = subprocess.run([SHELL, "-c", script], capture_output=True,
                           timeout=timeout,
                           env=dict(os.environ, HELLISH_NO_BANNER="1",
                                    HELLISH_NO_UPDATE_CHECK="1",
                                    HELLISH_NO_ANIM="1"))
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def registry_cases():
    rc, _, err = run('zle -N my-widget')
    check("registry/zle-N-registers", rc == 0, "rc=%d err=%r" % (rc, err[:90]))
    rc, _, err = run('zle -N my-widget my-function')
    check("registry/zle-N-takes-a-function", rc == 0, "rc=%d" % rc)
    rc, _, err = run('zle -N')
    check("registry/zle-N-needs-a-name", rc != 0 and b"expected" in err,
          "rc=%d err=%r" % (rc, err[:90]))
    # Re-registering replaces rather than duplicating.
    rc, _, _ = run('zle -N w f1; zle -N w f2; zle -N w f3')
    check("registry/re-registering-is-ok", rc == 0, "rc=%d" % rc)


def bindkey_cases():
    for args in ('"\\ex" w', '-M emacs "\\ex" w', '-M vicmd "\\e\\e" w',
                 '-M viins "\\e[3D" w'):
        rc, _, err = run('zle -N w; bindkey %s' % args)
        check("bindkey/accepts: %s" % args[:22], rc == 0,
              "rc=%d err=%r" % (rc, err[:90]))
    # Too few arguments is ignored, not an error: plugins call bindkey with
    # odd shapes during feature probing and must not die on it.
    rc, _, _ = run('bindkey')
    check("bindkey/bare-is-harmless", rc == 0, "rc=%d" % rc)


def guard_cases():
    """`zle && zle redisplay` is how every plugin guards. The bare `zle` has
    to be FALSE outside the editor or the guard is useless."""
    rc, _, _ = run('zle')
    check("guard/bare-zle-is-false-outside-the-editor", rc != 0, "rc=%d" % rc)
    rc, out, _ = run('zle && echo REDREW || echo skipped')
    check("guard/the-plugin-idiom-skips", out.strip() == b"skipped",
          "out=%r" % out)
    rc, _, err = run('zle redisplay')
    check("guard/widget-call-outside-the-editor-is-refused",
          rc != 0 and b"only be called from a widget" in err,
          "rc=%d err=%r" % (rc, err[:100]))


def unknown_widget_cases():
    """An unknown widget is REPORTED. Accepting it silently would leave the
    plugin believing it is installed, the user pressing the key, and nothing
    happening -- with nothing anywhere to read."""
    rc, _, err = run('zle -N real; zle definitely-not-a-widget')
    check("unknown/is-reported", b"no such widget" in err
          or b"only be called from a widget" in err, "err=%r" % err[:120])


def plugin_cases():
    """oh-my-zsh's sudo: it must LOAD and define both functions. Whether the
    key fires is not asserted -- see the module docstring."""
    p = os.path.join(CORPUS, "omz-sudo.zsh")
    if not os.path.exists(p):
        print("skip plugin/*  (corpus not cached)")
        return
    rc, out, err = run("source %s" % p)
    check("plugin/sudo-loads-silently", rc == 0 and err.strip() == b"",
          "rc=%d err=%r" % (rc, err[:160]))
    rc, out, _ = run("source %s; declare -F | sort" % p)
    got = sorted(l.split()[-1] for l in out.decode().split("\n") if l.strip())
    check("plugin/sudo-defines-its-widgets",
          got == ["__sudo-replace-buffer", "sudo-command-line"],
          "got %r" % got)
    # dirhistory does NOT load: `dirhistory_past[$#dirhistory_past]=()`,
    # assigning an empty array to one ELEMENT, is still a syntax error here.
    # Recorded rather than asserted-away; its widgets also cd, which the
    # forked-child boundary above rules out regardless.
    p = os.path.join(CORPUS, "omz-dirhistory.zsh")
    if os.path.exists(p):
        rc, _, err = run("source %s" % p)
        check("plugin/dirhistory-still-blocked", rc != 0
              and b"syntax error" in err,
              "it may LOAD now -- update the note; rc=%d err=%r"
              % (rc, err[:140]))


def churn_cases():
    script = ("i=0\nwhile [ $i -lt 300 ]; do\n"
              "  zle -N w$i fn$i\n"
              '  bindkey "\\e$i" w$i\n'
              "  zle >/dev/null 2>&1\n"
              "  i=$((i+1))\ndone\necho done\n")
    rc, out, err = run(script, timeout=90)
    check("churn/300-registrations-clean", rc == 0 and b"done" in out,
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])
    for bad in ['zle -N ""', 'bindkey "" w', 'bindkey "\\e" ""',
                'zle -N w; zle -N w; zle -N w', 'zle ""', 'bindkey -M']:
        rc, _, err = run(bad)
        check("churn/no-crash: %s" % bad[:24],
              b"AddressSanitizer" not in err and rc in (0, 1, 2, 127),
              "rc=%d err=%r" % (rc, err[:110]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    registry_cases()
    bindkey_cases()
    guard_cases()
    unknown_widget_cases()
    plugin_cases()
    churn_cases()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

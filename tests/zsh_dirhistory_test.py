#!/usr/bin/env python3
"""oh-my-zsh's dirhistory, driven end to end.

THE TWELFTH PLUGIN, and the one that needed the most: 1-based subscripts,
`$#name`, `arr[i]=()` as an element splice, `shift arrayname`,
`setopt localoptions no_ksh_arrays`, `${+terminfo[k]}`, and `add-zsh-hook
chpwd`. Loading it proves none of those individually; running its navigation
proves all of them at once, which is why this drives the plugin rather than
asserting on its parts.

WHAT THE EXPECTED VALUES ARE. Every stack below was taken from zsh 5.9 with
its own modules and functions loaded, running this same plugin -- not
reasoned out from the source. The internal stacks are asserted, not just
$PWD, because dirhistory recovers silently from a corrupt history: if
pop_past returns "" it decides someone overwrote its variable and resets to
$PWD. A wrong subscript base produces exactly that, and from the outside it
looks like a shell that simply did not move.

    hellish, zsh 5.9      after cd /tmp, /usr, /etc:
      back    /usr    past=[start /tmp /usr]    future=[/etc]
      back    /tmp    past=[start /tmp]         future=[/etc /usr]
      forward /usr    past=[start /tmp /usr]    future=[/etc]
      up      /

WHAT IS NOT ASSERTED: that ALT-LEFT does this. The binding is recorded,
translated and installed, and the dispatch fires -- a widget that edits the
BUFFER works, which is why oh-my-zsh's sudo does. But dirhistory's widgets
`cd`, readline runs in a FORKED CHILD (bg_readline), and a directory change
there dies with the child. Verified in a pty, not assumed: the key fires,
the widget runs, the parent shell stays put. Tracked as #80; asserting it
here would be a test passing against a binary where the feature does not
work.

Usage: python3 zsh_dirhistory_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
PLUGIN = os.path.join(os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus")), "omz-dirhistory.zsh")
FAILS = []
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")


def check(name, got, want):
    ok = got == want
    print(("ok   " if ok else "FAIL ") + name
          + ("" if ok else "\n       want %r\n       got  %r" % (want, got)))
    if not ok:
        FAILS.append(name)


def run(body, cwd):
    with tempfile.NamedTemporaryFile("w", suffix=".zsh", delete=False) as f:
        f.write("source %s\n%s\n" % (PLUGIN, body))
        path = f.name
    try:
        p = subprocess.run([SHELL, "-c", "source " + path], cwd=cwd,
                           capture_output=True, timeout=40, env=ENV)
        return p.stdout.decode(), p.stderr.decode(), p.returncode
    finally:
        os.unlink(path)


def load_cases(tmp):
    """It must load SILENTLY. A plugin that prints a diagnostic every time a
    shell starts is not usable even if everything after that line works."""
    out, err, rc = run('declare -F | wc -l', tmp)
    check("load/silent", (rc, err.strip()), (0, ""))
    check("load/defines-its-functions", int(out.strip() or 0) >= 12, True)
    out, _, _ = run('echo "[${dirhistory_past[@]}] $DIRHISTORY_SIZE"', tmp)
    check("load/seeds-past-with-cwd", out.strip(),
          "[%s] 30" % os.path.realpath(tmp))


def hook_cases(tmp):
    """add-zsh-hook chpwd is what fills the history. Without it the plugin
    loads and then does nothing at all: chpwd_dirhistory is the only caller
    of push_past."""
    out, _, _ = run('echo "[${chpwd_functions[@]}]"', tmp)
    check("hook/chpwd-registered", out.strip(), "[chpwd_dirhistory]")
    out, _, _ = run('cd /tmp; cd /usr; echo "[${dirhistory_past[@]}]"', tmp)
    check("hook/cd-appends-to-past", out.strip(),
          "[%s /tmp /usr]" % os.path.realpath(tmp))
    # The hook must not fire from inside itself: dirhistory's own helpers cd,
    # and an unguarded hook would recurse until the stack ran out.
    out, err, rc = run('loop() { cd /tmp; }\nadd-zsh-hook chpwd loop\n'
                       'cd /usr\necho SURVIVED', tmp)
    check("hook/reentrant-cd-does-not-recurse",
          (rc, "SURVIVED" in out, "AddressSanitizer" not in err),
          (0, True, True))


def nav_cases(tmp):
    """The navigation, against zsh 5.9's stacks at every step."""
    start = os.path.realpath(tmp)
    body = ('cd /tmp; cd /usr; cd /etc\n'
            'dirhistory_back\n'
            'echo "1|$PWD|${dirhistory_past[@]}|${dirhistory_future[@]}"\n'
            'dirhistory_back\n'
            'echo "2|$PWD|${dirhistory_past[@]}|${dirhistory_future[@]}"\n'
            'dirhistory_forward\n'
            'echo "3|$PWD|${dirhistory_past[@]}|${dirhistory_future[@]}"\n'
            'dirhistory_up\n'
            'echo "4|$PWD"\n')
    out, err, rc = run(body, tmp)
    got = [ln for ln in out.strip().split("\n") if "|" in ln]
    want = [
        "1|/usr|%s /tmp /usr|/etc" % start,
        "2|/tmp|%s /tmp|/etc /usr" % start,
        "3|/usr|%s /tmp /usr|/etc" % start,
        "4|/",
    ]
    check("nav/back-back-forward-up", got, want)
    check("nav/clean", (rc, err.strip()), (0, ""))


def cap_cases(tmp):
    """DIRHISTORY_SIZE caps the stack through `shift dirhistory_past`, which
    is the only thing in the corpus that shifts an array by name."""
    body = ('DIRHISTORY_SIZE=3\n'
            'cd /tmp; cd /usr; cd /etc; cd /var; cd /opt\n'
            'echo "n=$#dirhistory_past [${dirhistory_past[@]}]"\n')
    out, _, _ = run(body, tmp)
    check("cap/shift-holds-the-size", out.strip(), "n=3 [/etc /var /opt]")


def recover_cases(tmp):
    """The plugin's own corruption recovery must NOT trigger in normal use.
    It fires when pop_past hands back an empty string, which is precisely
    what a 0-based subscript produces for ${a[$#a]} -- so a passing
    navigation with a failing recovery check would mean the base is wrong
    and the plugin is quietly papering over it."""
    body = ('cd /tmp\ndirhistory_back\n'
            'echo "past=[${dirhistory_past[@]}]"\n')
    out, _, _ = run(body, tmp)
    check("recover/not-triggered-in-normal-use",
          out.strip(), "past=[%s]" % os.path.realpath(tmp))
    # And it DOES recover when the variable really is clobbered.
    body = ('cd /tmp\ndirhistory_past=()\ndirhistory_back\necho "$PWD"\n')
    out, err, rc = run(body, tmp)
    check("recover/clobbered-past-is-survivable",
          (rc, out.strip(), "AddressSanitizer" not in err),
          (0, "/tmp", True))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    if not os.path.exists(PLUGIN):
        print("skip: corpus not cached (%s)" % PLUGIN)
        return 0
    with tempfile.TemporaryDirectory() as tmp:
        load_cases(tmp)
        hook_cases(tmp)
        nav_cases(tmp)
        cap_cases(tmp)
        recover_cases(tmp)
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

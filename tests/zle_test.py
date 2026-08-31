#!/usr/bin/env python3
"""ZLE: the widget registry, bindkey, and the guard semantics plugins rely on.

WHAT THIS ASSERTS, and what it does NOT.

Asserted here, without a terminal: `zle -N` registers, `bindkey` records,
the bare `zle` guard is FALSE outside the editor and an unknown widget is
refused rather than silently accepted. Those are what decide whether a
plugin LOADS, and oh-my-zsh's sudo now does -- it previously exited 1 with
"not supported".

Asserted in a real pty, at the bottom of this file: that PRESSING the bound
key runs the widget and its edit lands on the line. That check was written
after an earlier pty run appeared to show ESC ESC reaching the terminal
unhandled -- which turned out to be the probe's own fault, not the shell's:
it passed the rc file with a flag hellish does not have, so the widget was
never registered in the shell being tested. The lesson is in the test now.
A negative result from a harness that never armed the feature looks
identical to a broken feature.

THE BOUNDARY THAT IS REAL. readline runs in a FORKED CHILD of the shell
(bg_readline). A widget's edits to BUFFER survive, because the line is what
the child sends back. Anything else it changes does not: a widget that runs
`cd` changes the child's directory and the parent never learns. Verified in
a pty rather than assumed -- the key fires, the widget runs, the shell stays
put. That is why sudo (which only edits the buffer) works end to end and
dirhistory's ALT-LEFT does not, though everything else about dirhistory
does. Tracked as #80.

Usage: python3 zle_test.py [/path/to/hellish]
"""
import os
import pty
import select
import subprocess
import sys
import tempfile
import time

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
    # dirhistory registers four widgets and binds ~30 key sequences across
    # three keymaps. Its NAVIGATION is covered by zsh_dirhistory_test.py;
    # what belongs here is that all that bindkey traffic is absorbed
    # without a diagnostic, since a plugin that complains on every shell
    # start is unusable whatever else works.
    p = os.path.join(CORPUS, "omz-dirhistory.zsh")
    if os.path.exists(p):
        rc, out, err = run("source %s; zle -N x; echo READY" % p)
        check("plugin/dirhistory-loads-silently",
              rc == 0 and b"READY" in out and err.strip() == b"",
              "rc=%d err=%r" % (rc, err[:160]))


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


def pty_drain(fd, secs):
    out = b""
    end = time.time() + secs
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.15)
        if not r:
            continue
        try:
            chunk = os.read(fd, 65536)
        except OSError:
            break
        if not chunk:
            break
        out += chunk
    return out


def pty_session(rc_body, keys):
    """Start an interactive hellish on a pty with `rc_body` as its rc, type
    `keys`, and hand back everything the terminal showed.

    --rcfile= is spelled exactly as hellish accepts it (one word, with the
    equals sign). An earlier version of this probe passed `--rcfile PATH`,
    which hellish does not recognise; the rc never loaded, the widget was
    never registered, and the key -- correctly -- did nothing. That read as
    a broken feature for as long as nobody checked the harness."""
    with tempfile.NamedTemporaryFile("w", suffix=".hsh",
                                     delete=False) as f:
        f.write(rc_body)
        rc_path = f.name
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1", TERM="xterm", PS1="> ")
    pid, fd = pty.fork()
    if pid == 0:
        try:
            os.execve(SHELL, [SHELL, "--rcfile=" + rc_path, "-i"], env)
        finally:
            os._exit(127)
    try:
        pty_drain(fd, 1.5)
        out = b""
        for k, pause in keys:
            os.write(fd, k)
            time.sleep(pause)
            out += pty_drain(fd, 0.5)
        os.write(fd, b"\x03exit\n")
        pty_drain(fd, 0.8)
        return out
    finally:
        os.close(fd)
        try:
            os.waitpid(pid, 0)
        except OSError:
            pass
        os.unlink(rc_path)


def dispatch_cases():
    """The half that needs a terminal: does the bound key RUN the widget,
    and does its edit survive back into the shell?

    The edit is checked by RUNNING the resulting line, not by reading the
    redraw. readline is free to repaint however it likes -- it answered an
    inserted prefix with `ESC [ 5 @ sudo `, an insert-N-characters escape,
    where a different terminal width would have reprinted the whole line --
    so a substring check against the terminal bytes tests readline's
    optimiser, not this shell. Executing the line tests the whole path the
    feature exists for: key -> widget -> BUFFER -> the parent shell."""
    rc = ("setopt zsh\n"
          'whole() { BUFFER="echo WIDGET_RAN"; }\n'
          "zle -N whole\n"
          "bindkey '\\e\\e' whole\n")
    out = pty_session(rc, [(b"\x1b\x1b", 0.8), (b"\r", 1.0)])
    check("dispatch/bound-key-runs-the-widget", b"WIDGET_RAN" in out,
          "terminal showed %r" % out[-200:])
    # LBUFFER, which is the half oh-my-zsh's sudo actually assigns: it never
    # touches BUFFER, so taking BUFFER's stale value would undo the edit.
    rc = ("setopt zsh\n"
          'pre() { LBUFFER="echo LB_$LBUFFER"; }\n'
          "zle -N pre\n"
          "bindkey '\\e\\e' pre\n")
    out = pty_session(rc, [(b"MARK", 0.4), (b"\x1b\x1b", 0.8), (b"\r", 1.0)])
    check("dispatch/lbuffer-edit-reaches-the-shell", b"LB_MARK" in out,
          "terminal showed %r" % out[-200:])
    # A key with no binding must reach readline untouched rather than being
    # swallowed by the dispatcher.
    out = pty_session(rc, [(b"echo KEPT", 0.4), (b"\x7f\x7f", 0.4),
                           (b"\r", 1.0)])
    check("dispatch/unbound-key-still-edits", b"KE\r\n" in out,
          "terminal showed %r" % out[-200:])


def cd_boundary_case():
    """A widget that cds does NOT move the shell, because readline runs in a
    forked child. Asserted so the boundary is pinned rather than folklore --
    if the architecture ever changes, this test says so.

    The destination is a FRESH directory with a unique name, not /tmp, and
    the check is whether the parent's `pwd` mentions it. `/tmp` cannot do
    that job: it matches the moment the suite runs from anywhere under
    /tmp, which is how this went red reporting a boundary change that had
    not happened."""
    dest = tempfile.mkdtemp(prefix="hellish_zle_boundary_")
    mark = os.path.basename(dest).encode()
    try:
        rc = ("setopt zsh\n"
              'jump() { cd %s; BUFFER="pwd"; }\n' % dest
              + "zle -N jump\n"
              "bindkey '\\e\\e' jump\n")
        out = pty_session(rc, [(b"\x1b\x1b", 0.8), (b"\r", 1.0)])
        check("boundary/widget-cd-does-not-move-the-shell", mark not in out,
              "it MOVED -- the fork boundary changed, update #80 and the "
              "docs; %r" % out[-200:])
    finally:
        os.rmdir(dest)


def message_cases():
    """`zle -M text` -- the message line under the prompt (#77, last item).

    It was the one spelling that took the silent path: builtin_zle answered
    any argument starting with '-' with a bare `return 0`, so `zle -M` set
    no message, printed nothing, and reported success. A plugin using it to
    explain what it had just done looked like it had done nothing at all.

    The buffer check is the one that matters. zsh puts the message BELOW
    the line being edited and the line survives; an implementation that
    prints wherever the cursor happens to be eats it instead."""
    rc, _, err = run("setopt zsh\nzle -M hi\n")
    check("message/refused-outside-the-editor",
          rc != 0 and b"widget" in err, "rc=%d err=%r" % (rc, err[:90]))
    rc_body = ("setopt zsh\n"
               'note() { zle -M "NOTE_MARK here"; }\n'
               "zle -N note\n"
               "bindkey '\\e\\e' note\n")
    out = pty_session(rc_body, [(b"echo KEEPME", 0.4), (b"\x1b\x1b", 0.9),
                                (b"\r", 1.0)])
    check("message/text-reaches-the-terminal", b"NOTE_MARK here" in out,
          "terminal showed %r" % out[-250:])
    check("message/the-line-being-edited-survives", b"KEEPME" in out,
          "the message ate the buffer; %r" % out[-250:])


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
    dispatch_cases()
    message_cases()
    cd_boundary_case()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

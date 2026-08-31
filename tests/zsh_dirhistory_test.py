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

ALT-LEFT IS ASSERTED TOO, at the bottom of this file, in a real pty.

It could not be until #80 item 2 landed. The binding was recorded,
translated and installed and the dispatch fired -- a widget that edits the
BUFFER worked, which is why oh-my-zsh's sudo did -- but dirhistory's widgets
`cd`, readline runs in a FORKED CHILD (bg_readline), and a directory change
there died with the child. The key fired, the widget ran, and the shell
stayed exactly where it was.

The child now reports its final directory to the parent on a pipe of its
own, and the parent adopts it, so the navigation completes. The pty case at
the bottom is the acceptance test for that: it presses the key a user would
press and asks the shell where it ended up. Everything above it drives the
plugin's functions directly, which is a different question -- those cases
would keep passing even if the key did nothing at all.

Usage: python3 zsh_dirhistory_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile
import time

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


def key_nav_case(tmp):
    """THE acceptance test for #80 item 2: press the key, move the shell.

    Everything else in this file calls dirhistory's functions directly, so
    it measures the plugin's logic and would pass unchanged against a build
    where the keybinding did nothing. This one goes through the whole chain
    -- readline dispatch, the widget, its `cd` in the forked child, and the
    child's report back to the parent -- and then asks the PARENT for $PWD,
    which is the only answer a user cares about.

    ESC [ 3 D is one of the four sequences dirhistory binds to
    dirhistory_zle_dirhistory_back (line 133 of the plugin: "xterm in normal
    mode"). Sent as raw bytes, exactly as a terminal would."""
    import pty
    import select

    a = os.path.join(tmp, "alpha")
    b = os.path.join(tmp, "beta")
    os.makedirs(a, exist_ok=True)
    os.makedirs(b, exist_ok=True)
    rc = os.path.join(tmp, "kn.zsh")
    with open(rc, "w") as f:
        f.write("source %s\n" % PLUGIN)

    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL, "-i", "--rcfile=" + rc],
                  dict(ENV, PS1="> ", TERM="xterm", HOME=tmp))
        os._exit(1)
    out = b""

    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d

    drain(2.0)
    for line in ("cd %s\n" % a, "cd %s\n" % b):
        os.write(fd, line.encode())
        drain(1.0)
    mark = len(out)
    os.write(fd, b"\x1b[3D")          # ALT-LEFT: dirhistory_back
    drain(1.5)
    os.write(fd, b"pwd\n")
    drain(1.5)
    tail = out[mark:]

    # The same key, pressed with a half-written command on the line. The
    # widget's first act is `zle .kill-buffer` and its last is
    # `zle .accept-line`; if the kill does not stick, accept-line submits
    # whatever the user had typed. It did -- pressing ALT-LEFT with `echo
    # DETONATE` on the line ran it. With a destructive command half-written
    # that is a very bad afternoon, so it is pinned on the plugin as well as
    # on the mechanism in zle_test.py.
    mark2 = len(out)
    os.write(fd, b"echo DETONATE")
    drain(0.6)
    os.write(fd, b"\x1b[3D")
    drain(1.5)
    tail2 = out[mark2:]
    try:
        os.write(fd, b"\x03")
        os.write(fd, b"exit\n")
        time.sleep(0.3)
        os.close(fd)
    except OSError:
        pass

    check("key/alt-left-moves-the-parent-shell",
          (os.path.realpath(a).encode() in tail
           or a.encode() in tail), True)
    check("key/alt-left-does-not-execute-a-half-typed-line",
          b"DETONATE\r\n" not in tail2, True)


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
        key_nav_case(tmp)
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

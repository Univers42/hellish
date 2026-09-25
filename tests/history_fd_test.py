#!/usr/bin/env python3
"""The history file's descriptor stays the shell's, and only the shell's.

An interactive shell streams each command into its history file through a
descriptor it keeps open. That descriptor was opened the plain way:

  - lowest free number, so fd 3, without close-on-exec: every program the
    shell ran inherited the history file open for writing (`ls -l
    /proc/self/fd` listed ~/.minishell_history);
  - in the 3..9 range scripts use themselves: after `exec 3>f` the next
    commands' history went into f -- the user's file -- and after
    `exec 3>&-` it went nowhere for the rest of the session.

bash never lends its history file to a child, and a user's redirection
never receives shell history. The fix opens it close-on-exec at fd >= 10
and remembers it by identity: a number the user has redirected or closed
is theirs from then on -- not written to, not closed -- and the history
file is opened again.

Each session waits on output markers, not on sleeps. Linux only (/proc).

Usage: python3 history_fd_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import shutil
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def read_file(path):
    try:
        with open(path, errors="replace") as f:
            return f.read()
    except OSError:
        return None


def session(home, rc, cmds):
    """Run cmds in an interactive shell; return its output, ANSI removed.
    Each command is followed by a marker echo that is waited for, so the
    next line is sent only once the previous one has finished."""
    with open(os.path.join(home, ".hellishrc"), "w") as f:
        f.write(rc)
    env = {
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "dumb", "LANG": "C.UTF-8", "PS1": "> ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL])
        os._exit(127)
    buf = bytearray()

    def wait_for(token, cap=15.0):
        end = time.monotonic() + cap
        while token not in buf and time.monotonic() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return False
            if not d:
                return False
            buf.extend(d)
        return token in buf

    ok = True
    for i, c in enumerate(cmds):
        os.write(fd, ('%s; echo "@@"MARK%d\n' % (c, i)).encode())
        ok = wait_for(b"@@MARK%d" % i) and ok
    os.write(fd, b"exit\n")
    end = time.monotonic() + 5
    while time.monotonic() < end:
        done, _ = os.waitpid(pid, os.WNOHANG)
        if done:
            break
        select.select([fd], [], [], 0.05)
        try:
            os.read(fd, 65536)
        except OSError:
            pass
    else:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    os.close(fd)
    out = re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]|[\x01\x02]", b"", bytes(buf))
    return ok, out.decode("utf-8", "replace")


def main():
    if not os.path.isdir("/proc/self/fd"):
        print("skip: no /proc/self/fd on this system")
        return 0

    home = tempfile.mkdtemp(prefix="hellish_histfd_")
    hist = os.path.join(home, ".minishell_history")
    u3 = os.path.join(home, "user3.txt")
    u10 = os.path.join(home, "user10.txt")
    try:
        ok, out = session(home, "", [
            "ls -l /proc/self/fd",
            "exec 3>%s" % u3,
            "echo HIST_A",
            "exec 10>%s" % u10,
            "echo HIST_B",
            "echo user-data >&10",
            "exec 10>&- 3>&-",
            "echo HIST_C",
        ])
        check("session ran to the end", ok, repr(out[-300:]))
        check("a child does not inherit the history file",
              ".minishell_history" not in out,
              repr([l for l in out.splitlines() if "history" in l]))
        check("exec 3>f: no history lands in the user's fd 3",
              read_file(u3) == "", repr(read_file(u3)))
        check("exec 10>f: the user's fd 10 holds only what they wrote",
              read_file(u10) == "user-data\n", repr(read_file(u10)))
        h = read_file(hist) or ""
        for want in ("echo HIST_A", "echo HIST_B", "echo HIST_C",
                     "exec 10>&- 3>&-"):
            check("history file records %r" % want, want in h, repr(h))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    # The rc takes over the descriptor, then moves the session to another
    # HISTFILE: the re-home must not close what is now the user's fd.
    home = tempfile.mkdtemp(prefix="hellish_histfd_")
    alt = os.path.join(home, "alt_hist")
    rc10 = os.path.join(home, "rc10.txt")
    try:
        ok, out = session(home, "exec 10>%s\nHISTFILE=%s\n" % (rc10, alt), [
            "echo rc-data >&10",
            "ls -l /proc/self/fd",
            "echo HIST_D",
        ])
        check("rehome session ran to the end", ok, repr(out[-300:]))
        check("rehome: the rc's fd 10 survives it",
              read_file(rc10) == "rc-data\n", repr(read_file(rc10)))
        check("rehome: a child inherits neither history file",
              "alt_hist" not in out and ".minishell_history" not in out,
              repr([l for l in out.splitlines() if "hist" in l]))
        a = read_file(alt) or ""
        check("rehome: HISTFILE records the session", "echo HIST_D" in a,
              repr(a))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

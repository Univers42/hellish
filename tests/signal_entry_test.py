#!/usr/bin/env python3
"""A signal ignored on entry cannot be trapped -- POSIX, and bash.

Whoever starts a shell decides which signals it may act on. `cmd &` sets
SIGINT and SIGQUIT to SIG_IGN in the child precisely so a ^C aimed at the
foreground job cannot kill the background one, and SIG_IGN survives
execve, so a shell started that way is handed a SIGINT it is not allowed
to take. cron, a systemd unit and a test harness that backgrounds the run
all do the same thing.

    Signals that were ignored on entry to the shell cannot be trapped or
    reset.                                            -- POSIX, trap

hellish installed the handler anyway. Found by running the golden suite
in the background: two cases against the pinned bash disagreed,

    trap "echo i" INT; trap -p INT
    bash:     trap -- '' INT
    hellish:  trap -- 'echo i' INT

and the handler really would have run, in a process its parent had
deliberately made deaf to that signal.

Every case here is run twice: once from a parent that ignores the signal
first (subprocess sets SIG_IGN in the child, before exec, which is what &
does), once normally as the control. Where bash is on PATH the same
scripts run under `bash --posix` and the outputs must match.

Usage: python3 signal_entry_test.py [/path/to/hellish]
"""
import os
import shutil
import signal
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
BASH = shutil.which("bash")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def ignore_int():
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGQUIT, signal.SIG_IGN)


def run(argv, script, ignored):
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "HOME": os.environ.get("HOME", "/tmp"), "TERM": "dumb",
           "LANG": "C.UTF-8", "HELLISH_NO_UPDATE_CHECK": "1",
           "HELLISH_BANNER": "0", "ASAN_OPTIONS": "detect_leaks=0"}
    p = subprocess.run(argv + ["-c", script], env=env, capture_output=True,
                       text=True, timeout=30,
                       preexec_fn=ignore_int if ignored else None)
    return p.returncode, p.stdout, p.stderr


# (name, script, what an ignoring parent must produce)
CASES = [
    ("a trap on an ignored signal is refused",
     "trap 'echo handler' INT; trap -p INT", "trap -- '' INT\n"),
    ("...and the attempt still succeeds, silently",
     "trap 'echo handler' INT; echo status=$?", "status=0\n"),
    ("resetting it is refused too",
     "trap 'echo handler' INT; trap - INT; trap -p INT", "trap -- '' INT\n"),
    ("both signals the & gave us are listed as ignored",
     "trap", "trap -- '' INT\ntrap -- '' QUIT\n"),
    ("the refused handler does not run",
     "trap 'echo HANDLER' INT; kill -INT $$; echo alive", "alive\n"),
    ("a signal that arrived normally is unaffected",
     "trap 'echo t' TERM; trap -p TERM", "trap -- 'echo t' TERM\n"),
]


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    for name, script, want in CASES:
        rc, out, err = run([SHELL], script, ignored=True)
        check(name, out == want and err == "",
              "out=%r err=%r want=%r" % (out, err, want))
        if BASH:
            _, bout, _ = run([BASH, "--posix"], script, ignored=True)
            check(name + " (bash agrees)", bout == want,
                  "bash=%r want=%r" % (bout, want))

    # The control: with nothing ignored, every one of those traps takes.
    ctl = [("the same trap takes when nothing was ignored",
            "trap 'echo handler' INT; trap -p INT",
            "trap -- 'echo handler' INT\n"),
           ("and it fires",
            "trap 'echo HANDLER' INT; kill -INT $$; echo alive",
            "HANDLER\nalive\n"),
           ("nothing is listed as ignored",
            "trap", "")]
    for name, script, want in ctl:
        rc, out, err = run([SHELL], script, ignored=False)
        check(name, out == want and err == "",
              "out=%r err=%r want=%r" % (out, err, want))
        if BASH:
            _, bout, _ = run([BASH, "--posix"], script, ignored=False)
            check(name + " (bash agrees)", bout == want,
                  "bash=%r want=%r" % (bout, want))

    print("\n%d failure(s)" % len(FAILS))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

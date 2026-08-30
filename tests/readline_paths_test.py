#!/usr/bin/env python3
"""Regression net for every libreadline entry point hellish uses.

The golden suite CANNOT reach any of this: every category runs through
`hellish -c`, which never enters the readline path at all. That blind spot is
why this file exists -- it is the gate for the dlopen-instead-of-link change,
where each of these symbols stops being a load-time relocation and becomes a
dlsym'd pointer:

  functions : readline, add_history, rl_completion_matches, rl_variable_bind
  data      : rl_attempted_completion_function, rl_completion_append_character,
              rl_editing_mode, rl_outstream, rl_instream, rl_point,
              rl_event_hook, rl_line_buffer

A dlsym that silently returns NULL, or a data pointer dereferenced one level
too few, shows up here as a hang, a missing completion, or a dead arrow key --
none of which any other test in the tree would notice.

Usage: python3 readline_paths_test.py /path/to/hellish
"""
import os
import pty
import re
import select
import signal
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def plain(b):
    """Strip CSI escapes so assertions match on visible text only."""
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b).decode(errors="replace")


class Session:
    def __init__(self, home):
        env = {
            "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": "xterm-256color", "LANG": "C.UTF-8",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_NO_ANIM": "1",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
        os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
        open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.environ.clear()
            os.environ.update(env)
            # --norc: pin the config. An inherited ~/.hellishrc can set PS1 or
            # define names, and quietly decide what this test sees.
            os.execvp(SHELL, [SHELL, "--norc"])
            os._exit(127)
        self.drain(0.8)

    def drain(self, t=0.35):
        out = b""
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.08)
            if r:
                try:
                    c = os.read(self.fd, 65536)
                except OSError:
                    break
                if not c:
                    break
                out += c
        return out

    def send(self, s, wait=0.40):
        os.write(self.fd, s.encode())
        return self.drain(wait)

    def close(self):
        """exit; True when the shell terminated by itself (no wedge)."""
        self.send("exit\n", 0.5)
        for _ in range(30):
            try:
                p, _ = os.waitpid(self.pid, os.WNOHANG)
            except ChildProcessError:
                return True
            if p:
                return True
            time.sleep(0.1)
        try:
            os.kill(self.pid, signal.SIGKILL)
            os.waitpid(self.pid, 0)
        except (ProcessLookupError, ChildProcessError):
            pass
        return False


def main():
    home = tempfile.mkdtemp(prefix="hellish-rl-")
    # Distinctly-named files so a completion match is unambiguous.
    os.makedirs(os.path.join(home, "cdir"), exist_ok=True)
    for n in ("zzunique_alpha.txt", "zzunique_beta.txt"):
        open(os.path.join(home, n), "w").close()

    s = Session(home)
    s.send("cd %s\n" % home, 0.5)   # fixtures below live here

    # Guard: a pty ECHOES typed input, so a literal marker would "pass" even
    # with a dead shell. Every check below asserts on an EXPANSION result the
    # terminal cannot have echoed, and this first one also proves we are live.

    # --- readline() itself: the shell reads a line and runs it -------------
    out = plain(s.send("echo RL_$((6*7))_OK\n"))
    check("readline: reads and executes a line", "RL_42_OK" in out, repr(out[-120:]))

    # --- add_history + arrow keys: recall the previous line ----------------
    s.send("echo HIST_$((5*5))\n")
    out = plain(s.send("\x1b[A", 0.5))          # Up arrow
    recalled = "echo HIST_$((5*5))" in out
    out2 = plain(s.send("\n", 0.5))
    check("add_history: up-arrow recalls and re-runs",
          recalled and "HIST_25" in out2, repr(out[-120:]))

    # --- completion: command name (rl_attempted_completion_function
    #     -> rl_completion_matches) ---------------------------------------
    s.send("ech\t", 0.6)
    out = plain(s.send(" CMPL_$((2*3))\n", 0.6))
    check("completion: command name completes (echo runs)", "CMPL_6" in out,
          repr(out[-160:]))

    # --- completion: filename + rl_completion_append_character ------------
    s.send("echo FN_ zzunique_al\t", 0.6)
    out = plain(s.send("\n", 0.6))
    check("completion: filename completes", "zzunique_alpha.txt" in out,
          repr(out[-200:]))

    # --- completion: ambiguous prefix lists both (rl_completion_matches) ---
    out = plain(s.send("echo zzunique_\t\t", 0.9))
    both = "zzunique_alpha.txt" in out and "zzunique_beta.txt" in out
    check("completion: ambiguous prefix lists candidates", both, repr(out[-260:]))
    s.send("\x15", 0.3)

    # --- rl_variable_bind / rl_editing_mode: vi then back to emacs ---------
    s.send("set -o vi\n")
    out = plain(s.send("echo VI_$((3*3))\n"))
    check("set -o vi: shell still executes", "VI_9" in out, repr(out[-120:]))

    s.send("set -o emacs\n")
    out = plain(s.send("echo EM_$((4*4))\n"))
    check("set -o emacs: shell still executes", "EM_16" in out, repr(out[-120:]))

    # emacs editing keys must still work after the round trip (rl_point /
    # rl_line_buffer): ^A jumps home, so the typed text lands after "echo ".
    out = plain(s.send("$((7*8))\x01echo \n", 0.6))
    check("emacs keys live after mode round-trip (^A)", "56" in out, repr(out[-140:]))

    check("session exits cleanly (no readline wedge)", s.close())

    print("== ALL PASSED ==" if not FAILS else "== %d FAILED: %s ==" %
          (len(FAILS), ", ".join(FAILS)))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())

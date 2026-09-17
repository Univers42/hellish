#!/usr/bin/env python3
"""Regression gate: time-to-prompt stays within a bound of bash's, in a pty.

Nothing in bench/ or tests/ started an interactive shell before this file:
every benchmark ran `-c` or a script, and HELLISH_PROMPT_BENCH timed the
string renderer alone. The user's complaint -- "the initialization of the
input each time we prompt is really slow", prompts lagging behind a held
Enter key -- lived entirely in the part nobody measured: the readline fork
and its per-prompt re-initialisation, hook dispatch through the whole
parser, a git status spawned every prompt, an update-state file read every
cycle.

A BURST of Enters is the workload on purpose: it is the user's report
("holding Enter prints stray space and the prompt lags behind") turned into
a number, and it is where a per-prompt fork shows up as something other than
a rounding error.

WHAT THIS ASSERTS -- a ratio against the bash in PATH measured in the SAME
run, OR an absolute per-prompt delta, whichever the shell satisfies. Neither
alone works here. A pure ratio is unusable because bash's own median is
~0.07 ms per prompt on this workload: 1.5x of that is below the pty
round-trip floor, so the bound would measure the harness, not the shell. A
pure absolute is the flaky thing the suite already avoids
(func_registry_perf_test.py). So: ratio <= BOUND *or* hellish-minus-bash
<= DELTA_MS per prompt. Median of ROUNDS bursts, because one scheduler
hiccup must not decide a verdict.

Run it on a RELEASE build. ASan roughly quadruples hellish's side and does
nothing to bash's, which turns a fair comparison into a fixed 4x penalty --
`make prompt-latency-test` builds OPT=1 for exactly this reason.

Measured on a release build when this file was written (bash 5.3.9 oracle,
pty 24x100, cwd = dirty git repo):

    bash --norc                            0.07 ms per prompt
    hellish --norc                         0.96 ms per prompt   13.6x
    bash --rcfile <twin of the fixture>    0.16 ms per prompt
    hellish + fixtures/frontend.hellishrc  1.21 ms per prompt    7.4x

After the line was read in the shell process (3.2), same machine:

    bash --norc                            0.06 ms per prompt
    hellish --norc                         0.07 ms per prompt    1.3x
    bash --rcfile <twin of the fixture>    0.11 ms per prompt
    hellish + fixtures/frontend.hellishrc  0.14 ms per prompt    1.3x

Per bare Enter the original cost was: one fork for readline plus a full
rl_initialize() in the child (terminfo database, ~/.inputrc and /etc/inputrc
re-parsed EVERY prompt -- see prompt_syscall_budget_test.py), a `git status`
subprocess, five hook dispatches through the whole lexer and parser, and an
update-state file read. The bounds below are the goal those fixes are aimed
at, not a description of today.

HELLISH_LATENCY_RC=/path/to/.hellishrc adds an informational row for a real
configuration (its sibling ~/.hellish directory is copied along); it never
fails the test.

Usage: python3 prompt_latency_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import statistics
import struct
import subprocess
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURE = os.path.join(HERE, "fixtures", "frontend.hellishrc")
BASH = shutil.which("bash")
GIT = shutil.which("git")
ROUNDS = 15
BURST = 20
# A RATCHET, not a wish. Each bound sits just above what the shell actually
# achieves today, so a regression fails immediately; each phase that removes
# work tightens them. Setting them straight to the goal would leave a gate
# that is red for weeks, and a permanently red gate is one nobody reads.
#
#   measured, release build, this machine     bare        configured
#   before any of this work                  13.60x        7.40x
#   readline pre-initialised in the parent
#     + git rescan gated on a real change     8.50x        5.14x
#   the line read in the shell process        1.30x        1.29x
#     (0.07 ms against bash's 0.06 per prompt, 0.14 against 0.11)
#
# At that size a ratio is mostly harness noise, so the bounds sit at 2x
# rather than at the measured figure; DELTA_MS catches the rest.
BARE_BOUND = 2.0
RC_BOUND = 2.0
# Per-prompt milliseconds hellish may exceed bash by regardless of the ratio.
# Sized so that a shell which no longer forks per prompt passes on this rule
# alone even when bash's own median is too small for a ratio to mean anything.
DELTA_MS = 0.30
MARK_BARE = b"HX> "
MARK_RC = "❯".encode()
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + (" " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def base_env(home):
    return {
        "HOME": home, "XDG_CONFIG_HOME": os.path.join(home, ".config"),
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null",
    }


class Session:
    def __init__(self, argv, env, cwd):
        self.buf = bytearray()
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(cwd)
            os.environ.clear()
            os.environ.update(env)
            os.execvp(argv[0], argv)
            os._exit(127)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                    struct.pack("HHHH", 24, 100, 0, 0))

    def drain(self, cap=6.0, quiet=0.4):
        last = time.monotonic()
        end = last + cap
        while time.monotonic() < end:
            r, _, _ = select.select([self.fd], [], [], 0.03)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                except OSError:
                    return
                if not d:
                    return
                self.buf.extend(d)
                last = time.monotonic()
            elif time.monotonic() - last > quiet:
                return

    def burst(self, marker, n=BURST, timeout=15.0):
        """Send n Enters at once; seconds until n more prompts have been
        drawn (marker counted), or None."""
        n0 = self.buf.count(marker)
        t0 = time.monotonic()
        os.write(self.fd, b"\r" * n)
        while time.monotonic() - t0 < timeout:
            r, _, _ = select.select([self.fd], [], [], 0.01)
            if not r:
                continue
            try:
                self.buf.extend(os.read(self.fd, 65536))
            except OSError:
                return None
            if self.buf.count(marker) >= n0 + n:
                return time.monotonic() - t0
        return None

    def close(self):
        try:
            os.write(self.fd, b"exit\r")
            self.drain(1.0)
            os.kill(self.pid, 9)
        except OSError:
            pass
        os.waitpid(self.pid, 0)


def make_repo(base):
    """A small dirty git repo, so the \\g badge is live and the git status
    check has something to look at -- the shape of a developer's cwd."""
    d = os.path.join(base, "repo")
    os.makedirs(d)
    if not GIT:
        return d
    env = {"PATH": os.environ["PATH"], "HOME": base,
           "GIT_CONFIG_GLOBAL": "/dev/null", "GIT_CONFIG_SYSTEM": "/dev/null"}
    run = lambda *a: subprocess.run(
        [GIT, "-C", d] + list(a), env=env, check=True,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    run("init", "-q", "-b", "main")
    open(os.path.join(d, "f.txt"), "w").write("x\n")
    run("add", "f.txt")
    run("-c", "user.email=t@t", "-c", "user.name=t", "commit", "-qm", "i")
    open(os.path.join(d, "f.txt"), "a").write("y\n")
    return d


def fixture_ps1():
    for line in open(FIXTURE, encoding="utf-8"):
        if line.startswith("PS1="):
            return line[len("PS1="):].strip().strip("'")
    raise SystemExit("fixture has no PS1")


def bash_twin_rc(home):
    """bash's version of the fixture: same PS1 text (bash escapes, git badge
    dropped since \\g is hellish's), the same three hook functions run from
    PROMPT_COMMAND, multi-line history."""
    ps1 = fixture_ps1().replace("\\g", "").replace("\\S", "")
    path = os.path.join(home, ".bashrc_twin")
    with open(path, "w", encoding="utf-8") as f:
        f.write("PS1='%s'\n" % ps1)
        f.write("fx_precmd_a() { FX_A=$((${FX_A:-0} + 1)); }\n"
                "fx_precmd_b() { FX_B=$((${FX_B:-0} + 1)); }\n"
                "fx_prompt_command() { FX_PC=$((${FX_PC:-0} + 1)); }\n"
                "PROMPT_COMMAND='fx_precmd_a; fx_precmd_b; fx_prompt_command'\n"
                "shopt -s cmdhist lithist\n")
    return path


def measure(label, argv, env, cwd, marker):
    s = Session(argv, env, cwd)
    s.drain(8.0)
    if s.buf.count(marker) == 0:
        check("%s: prompt appeared" % label, False,
              "no marker %r in %r" % (marker, bytes(s.buf[-300:])))
        s.close()
        return None
    times = []
    for _ in range(ROUNDS):
        t = s.burst(marker)
        if t is None:
            check("%s: burst of %d Enters completes" % (label, BURST), False,
                  "timed out; tail=%r" % bytes(s.buf[-200:]))
            s.close()
            return None
        times.append(t)
        s.drain(1.0, quiet=0.25)
    s.close()
    med = statistics.median(times)
    print("     %-44s median %6.1f ms / %d prompts = %5.2f ms per prompt"
          % (label, med * 1000, BURST, med * 1000 / BURST))
    return med


def verdict(label, hx, bash, bound):
    """Pass on the ratio OR on the absolute per-prompt delta -- see the
    module docstring for why neither rule works on its own here."""
    if None in (hx, bash):
        return
    ratio = hx / bash
    delta = (hx - bash) * 1000 / BURST
    print("     %-20s ratio %5.2fx, %+.2f ms per prompt vs bash"
          % (label, ratio, delta))
    check("%s: within %.1fx of bash, or +%.2f ms per prompt"
          % (label, bound, DELTA_MS),
          ratio <= bound or delta <= DELTA_MS,
          "ratio %.2fx and %+.2f ms per prompt" % (ratio, delta))


def main():
    base = tempfile.mkdtemp(prefix="hellish_latency_")
    home = os.path.join(base, "home")
    os.makedirs(home)
    cwd = make_repo(base)
    if not BASH:
        print("FAIL bash not found; the ratio needs a control")
        sys.exit(1)

    print("time-to-prompt, %d bursts of %d Enters, median (pty 24x100, cwd = dirty git repo)"
          % (ROUNDS, BURST))
    bash_bare = measure("bash --norc", [BASH, "--norc", "--noprofile"],
                        dict(base_env(home), PS1=MARK_BARE.decode()), cwd, MARK_BARE)
    hx_bare = measure("hellish --norc", [SHELL, "--norc"],
                      dict(base_env(home), PS1=MARK_BARE.decode()), cwd, MARK_BARE)
    twin = bash_twin_rc(home)
    bash_rc = measure("bash --rcfile <twin of fixture>",
                      [BASH, "--noprofile", "--rcfile", twin],
                      base_env(home), cwd, MARK_RC)
    shutil.copy(FIXTURE, os.path.join(home, ".hellishrc"))
    hx_rc = measure("hellish + fixtures/frontend.hellishrc", [SHELL],
                    base_env(home), cwd, MARK_RC)
    os.unlink(os.path.join(home, ".hellishrc"))

    verdict("bare prompt", hx_bare, bash_bare, BARE_BOUND)
    verdict("configured prompt", hx_rc, bash_rc, RC_BOUND)

    real = os.environ.get("HELLISH_LATENCY_RC")
    if real and os.path.isfile(real):
        rhome = os.path.join(base, "realhome")
        os.makedirs(rhome)
        shutil.copy(real, os.path.join(rhome, ".hellishrc"))
        sib = os.path.join(os.path.dirname(os.path.abspath(real)), ".hellish")
        if os.path.isdir(sib):
            shutil.copytree(sib, os.path.join(rhome, ".hellish"))
        mark = os.environ.get("HELLISH_LATENCY_MARK", "❯").encode()
        measure("hellish + %s (informational)" % real, [SHELL],
                base_env(rhome), cwd, mark)

    shutil.rmtree(base, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()

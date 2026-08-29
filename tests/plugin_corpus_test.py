#!/usr/bin/env python3
"""Real third-party plugins, as a test.

The only thing that can tell us whether the zsh and rc work actually
succeeded. Synthetic cases pass because we wrote both sides; a plugin
somebody else wrote in 2013 does not care what we assumed.

That has already paid for itself twice. Sourcing git's own git-prompt.sh
segfaulted the shell -- a bug in t_scope_save that existed only in RELEASE
builds while the golden suite was passing 3790/3790 in debug. And sourcing
oh-my-zsh's git plugin, which defines 201 aliases, put 18 KB of leaked
memory on the ft_malloc oracle from a destructor shutdown never called.
Neither was reachable from anything we would have written ourselves.

HOW IT STAYS HONEST

Every plugin declares an EXPECTATION. A plugin that starts working is a
FAILURE until its expectation is updated -- so the matrix cannot rot into
"most of these probably still work". The expectations are:

    loads          sources cleanly, exit status 0, no stderr
    loads-noisy    sources and defines things, but says something on
                   stderr about a feature we do not have
    unsupported    cannot work here, with the reason recorded -- asserted
                   as "fails out loud", not as a particular message, so
                   closing a neighbouring gap does not turn it red

VENDORS NOTHING. Downloads to a cache under $XDG_CACHE_HOME and SKIPS
CLEANLY when offline or when nothing is cached, so CI without a network
still passes rather than pretending to have checked.

RUNS AGAINST WHATEVER IS BUILT. A crash while sourcing arbitrary input is a
hellish bug, never the plugin's -- so the checks that matter (no segfault,
no sanitizer report) are asserted on every build, and `make plugin-corpus`
runs it against release and ASan in turn.

Usage: python3 plugin_corpus_test.py [/path/to/hellish]
       PLUGIN_CACHE=<dir>   where to keep downloads
       OFFLINE=1            never touch the network
"""
import os
import subprocess
import sys
import urllib.error
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
CACHE = os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus"))
OFFLINE = os.environ.get("OFFLINE", "") not in ("", "0")
FAILS = []
SKIPPED = []

OMZ = ("https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/plugins/%s")
GIT = ("https://raw.githubusercontent.com/git/git/master/contrib/%s")

# name, url, expectation, note
#
# `min_defs` is the floor on what the plugin must DEFINE (functions plus
# aliases). Loading without error while defining nothing is the failure this
# catches: an early `return` in a guard we got wrong looks identical to
# success from the outside.
CORPUS = [
    ("omz-git", OMZ % "git/git.plugin.zsh", "loads-noisy", 200,
     "compdef: no zsh completion system here"),
    ("omz-sudo", OMZ % "sudo/sudo.plugin.zsh", "unsupported", 0,
     "drives ZLE (zle/bindkey); readline has no widget model"),
    ("omz-extract", OMZ % "extract/extract.plugin.zsh", "loads", 1, ""),
    ("omz-web-search", OMZ % "web-search/web-search.plugin.zsh", "loads", 5,
     ""),
    ("omz-copypath", OMZ % "copypath/copypath.plugin.zsh", "loads", 1, ""),
    ("omz-dirhistory", OMZ % "dirhistory/dirhistory.plugin.zsh",
     "unsupported", 0, "50 ZLE calls; also arr[i]=() element assignment"),
    ("omz-jsontools", OMZ % "jsontools/jsontools.plugin.zsh", "loads", 3, ""),
    ("omz-colored-man", OMZ % "colored-man-pages/colored-man-pages.plugin.zsh",
     "loads", 4, ""),
    ("git-prompt", GIT % "completion/git-prompt.sh", "loads", 4,
     "the one that used to segfault"),
    ("git-completion", GIT % "completion/git-completion.bash", "loads-noisy",
     100, "compgen: no programmable completion here"),
    ("bash-preexec",
     "https://raw.githubusercontent.com/rcaloras/bash-preexec/master/"
     "bash-preexec.sh", "loads", 15, ""),
    ("z", "https://raw.githubusercontent.com/rupa/z/master/z.sh", "loads", 1,
     ""),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def fetch(name, url):
    """Cached download. Returns a path, or None when it cannot be had --
    which is a SKIP, not a failure: a machine without a network has not
    told us anything about the shell."""
    ext = ".zsh" if url.endswith(".zsh") else os.path.splitext(url)[1]
    path = os.path.join(CACHE, name + (ext or ".sh"))
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return path
    if OFFLINE:
        return None
    try:
        os.makedirs(CACHE, exist_ok=True)
        with urllib.request.urlopen(url, timeout=20) as r:
            data = r.read()
        if not data:
            return None
        with open(path, "wb") as f:
            f.write(data)
        return path
    except (urllib.error.URLError, OSError, ValueError):
        return None


def run(script, timeout=45):
    env = dict(os.environ, HELLISH_NO_BANNER="1",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1")
    try:
        p = subprocess.run([SHELL, "-c", script], capture_output=True,
                           timeout=timeout, env=env)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return -9, b"", b"<timeout>"
    except OSError as e:
        return -1, b"", str(e).encode()


def defines(path):
    """How many functions and aliases the plugin left behind."""
    rc, out, _ = run("source %s >/dev/null 2>&1\n"
                     "echo $(( $(declare -F 2>/dev/null | wc -l) "
                     "+ $(alias 2>/dev/null | wc -l) ))" % path)
    try:
        return int(out.decode().strip().split()[-1])
    except (ValueError, IndexError):
        return -1


def crash_words(err, rc):
    """A crash is a hellish bug whatever the input. Signals show up as a
    negative return code from the shell we spawned, and the sanitizers
    announce themselves by name."""
    bad = []
    if rc < 0 and rc != -9:
        bad.append("signal %d" % -rc)
    for w in (b"AddressSanitizer", b"LeakSanitizer", b"Sanitizer",
              b"Segmentation fault", b"core dumped"):
        if w in err:
            bad.append(w.decode())
    return bad


def one(name, url, expect, min_defs, note):
    path = fetch(name, url)
    if not path:
        SKIPPED.append(name)
        print("skip %-18s (not cached%s)"
              % (name, ", offline" if OFFLINE else " and no network"))
        return
    rc, out, err = run("source %s" % path)

    # 1. Non-negotiable, whatever the expectation: sourcing arbitrary input
    #    must not crash the shell and must not trip a sanitizer.
    bad = crash_words(err, rc)
    check("%s/no-crash" % name, not bad, "%s; err=%r" % (bad, err[:200]))
    check("%s/terminates" % name, rc != -9, "timed out")

    # 2. The declared expectation.
    if expect == "loads":
        check("%s/loads-silently" % name, rc == 0 and err.strip() == b"",
              "rc=%d err=%r" % (rc, err[:200]))
    elif expect == "loads-noisy":
        check("%s/loads" % name, rc == 0,
              "rc=%d err=%r" % (rc, err[:200]))
        check("%s/says-what-is-missing" % name, err.strip() != b"",
              "expected a message about %s, got silence" % note)
        check("%s/message-is-ours-not-a-crash" % name,
              b"not supported" in err or b"command not found" in err,
              "err=%r" % err[:200])
    elif expect == "unsupported":
        # The contract for an unsupported plugin is only that it FAILS
        # HONESTLY: it says something and it does not crash. Requiring a
        # particular message would be asserting which construct it trips on
        # first, and that changes every time a neighbouring gap is closed --
        # a test that goes red on progress teaches you to ignore it.
        check("%s/fails-out-loud" % name, err.strip() != b"",
              "expected a message about %s, got silence" % note)
        check("%s/no-partial-nonsense" % name, rc != 0,
              "reported %r but exited 0" % err[:120])

    # 3. It must actually DEFINE something. Loading cleanly while defining
    #    nothing is the failure that looks most like success.
    if min_defs > 0:
        n = defines(path)
        check("%s/defines-at-least-%d" % (name, min_defs), n >= min_defs,
              "defined %d" % n)


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    print("--- shell: %s" % SHELL)
    mode = os.path.join(ROOT, "build", "bin", ".mode")
    if os.path.exists(mode):
        print("--- build: %s" % open(mode).read().strip())
    print("--- cache: %s%s" % (CACHE, "  (offline)" if OFFLINE else ""))
    for row in CORPUS:
        one(*row)
    if SKIPPED and len(SKIPPED) == len(CORPUS):
        print("\nSKIP: nothing cached and no network -- corpus not exercised")
        return 0
    if SKIPPED:
        print("\nskipped (not cached): %s" % " ".join(SKIPPED))
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

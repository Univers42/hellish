#!/usr/bin/env python3
"""The plugin framework (hxp), end to end, as a test.

What the installer actually gives a user is not one plugin file: it is
~/.polishrc, the framework under ~/.polish (lib, rc.d, the catalog), the
builtin plugins it ships, the oh-my-zsh and git plugins it fetches, and a
polish.conf that switches them on (an install from before the 3.4.0
rename keeps ~/.hellishrc, ~/.hellish and hellish.conf, and this test takes
either layout). plugin_corpus_test.py proves each third-party FILE
loads; this proves the CONFIGURATION a user ends up with loads, passes
the framework's own suite, answers its management commands, and survives
using what the plugins define.

WHY IT EXISTS. With every default plugin on, `man bash` segfaulted the
interactive shell (exit 139) on a freshly installed machine. The plugin
(colored-man-pages) had loaded cleanly -- the corpus row said so -- but its
`man` wrapper appends `PAGER="..."` to an array, and an array element shaped
like an assignment crashed the expander. Nothing loaded the whole config
and then USED it, so nothing could have noticed. This does.

WHERE THE FRAMEWORK COMES FROM, in order:
    HXP_SRC=<checkout>     a clone of github.com/Univers42/hellishrc_plugins:
                           its own install.sh --home builds the sandbox
    HXP_HOME=<dir>         an installed ~/.polish or ~/.hellish (its
                           .polishrc / .hellishrc must sit beside it); the
                           default is $HOME/.polish, then $HOME/.hellish,
                           when one exists -- so on a developer's machine
                           this tests the configuration they actually run
    the cache              a tarball of the repo under $PLUGIN_CACHE,
                           downloaded once unless OFFLINE=1
    nothing                SKIP, out loud. A machine without the framework
                           and without a network has proven nothing.

Everything runs in a throwaway HOME: the real framework is copied, never
touched, and the shell's own rc.d is not on the path, so the verdict is about
the framework and not about whatever else this machine has configured.

Usage: python3 hxp_framework_test.py [/path/to/polish]
       PLUGIN_CACHE=<dir>   where to keep the framework tarball
       OFFLINE=1            never touch the network
"""
import io
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "polish"))
CACHE = os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus"))
OFFLINE = os.environ.get("OFFLINE", "") not in ("", "0")
REPO = "https://github.com/Univers42/hellishrc_plugins"
TARBALL = REPO + "/archive/refs/heads/main.tar.gz"
FAILS = []
# (framework dir, loader) per brand, new name first: the installer picks the
# polish pair unless only the legacy pair exists, and so does this test.
LAYOUTS = ((".polish", ".polishrc"), (".hellish", ".hellishrc"))


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def crash_words(err, rc):
    bad = []
    if rc < 0 and rc != -9:
        bad.append("signal %d" % -rc)
    if rc >= 128:
        bad.append("exit %d" % rc)
    for w in (b"AddressSanitizer", b"LeakSanitizer", b"Sanitizer",
              b"Segmentation fault", b"core dumped", b"ft_assert"):
        if w in err:
            bad.append(w.decode())
    return bad


# ---------------------------------------------------------------- sandbox

def sandbox_env(home):
    """A HOME of our own and nothing inherited that could steer the load:
    the developer's HX_* registry, HELLISH_EXECD, a PAGER that waits for a
    keypress. XDG_CONFIG_HOME points inside the sandbox so the shell's own
    rc.d and after.d (90-zshrc.zsh would source the real ~/.zshrc) stay out.
    HXP_RC names the loader the sandbox ended up with."""
    rc = layout(home)
    return {
        "HXP_RC": os.path.join(home, rc[1]) if rc else "",
        "HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "TERM": "xterm", "LC_ALL": "C", "USER": os.environ.get("USER", "u"),
        "XDG_CONFIG_HOME": os.path.join(home, ".config"),
        "XDG_CACHE_HOME": os.path.join(home, ".cache"),
        "POLISH_NO_BANNER": "1", "POLISH_NO_UPDATE_CHECK": "1",
        "POLISH_NO_ANIM": "1",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "PAGER": "cat", "MANPAGER": "cat",
        "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", ""),
    }


def layout(home):
    """The (framework dir, loader) pair installed under home, or None."""
    for pair in LAYOUTS:
        if (os.path.isdir(os.path.join(home, pair[0], "lib"))
                and os.path.isfile(os.path.join(home, pair[1]))):
            return pair
    return None


def installed_framework():
    """(dir, loader path) of the developer's install, or None. HXP_HOME
    may be any directory; its loader is the brand rc beside it."""
    if os.environ.get("HXP_HOME"):
        fw = os.environ["HXP_HOME"].rstrip("/")
        cands = [os.path.join(os.path.dirname(fw), rc) for _, rc in LAYOUTS]
    else:
        home = os.path.expanduser("~")
        pair = layout(home)
        if not pair:
            return None
        fw = os.path.join(home, pair[0])
        cands = [os.path.join(home, pair[1])]
    for rc in cands:
        if os.path.isdir(os.path.join(fw, "lib")) and os.path.isfile(rc):
            return fw, rc
    return None


def run(home, script, cwd=None, timeout=180, argv=None):
    env = sandbox_env(home)
    cmd = [SHELL] + (argv if argv else ["-c", script])
    try:
        p = subprocess.run(cmd, capture_output=True, timeout=timeout,
                           env=env, cwd=cwd or home)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return -9, b"", b"<timeout>"
    except OSError as e:
        return -1, b"", str(e).encode()


def fetch_checkout():
    """A checkout of the framework repo in the cache, from the tarball
    (plain HTTPS: no git transport to negotiate). None means skip."""
    dst = os.path.join(CACHE, "hellishrc_plugins")
    if os.path.isfile(os.path.join(dst, "install.sh")):
        return dst
    if OFFLINE:
        return None
    try:
        os.makedirs(CACHE, exist_ok=True)
        with urllib.request.urlopen(TARBALL, timeout=30) as r:
            data = r.read()
        with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
            top = tf.getmembers()[0].name.split("/")[0]
            tf.extractall(CACHE)
        os.rename(os.path.join(CACHE, top), dst)
        return dst if os.path.isfile(os.path.join(dst, "install.sh")) else None
    except (urllib.error.URLError, OSError, ValueError, tarfile.TarError):
        return None


def build_sandbox(home):
    """Returns a short description of where the framework came from, or
    None when there is nothing to test."""
    src = os.environ.get("HXP_SRC")
    installed = installed_framework()
    if src and os.path.isfile(os.path.join(src, "install.sh")):
        origin = "checkout %s" % src
    elif installed:
        fw, rc = installed
        # keep the brand of the loader: a .hellishrc sources ~/.hellish
        pair = LAYOUTS[1] if rc.endswith(".hellishrc") else LAYOUTS[0]
        shutil.copytree(fw, os.path.join(home, pair[0]), symlinks=True)
        shutil.copy(rc, os.path.join(home, pair[1]))
        return "installed %s" % fw
    else:
        src = fetch_checkout()
        if not src:
            return None
        origin = "cached checkout %s" % src
    p = subprocess.run(["sh", os.path.join(src, "install.sh"), "--home", home,
                        "--plugins", "all"], capture_output=True,
                       timeout=600, env=sandbox_env(home))
    if p.returncode != 0 or not layout(home):
        print("framework installer failed: rc=%d %r" % (p.returncode,
                                                        p.stderr[-300:]))
        return None
    return origin


# ------------------------------------------------------------------ checks

LOAD = ('. "$HXP_RC"\n'
        'printf "loaded=%s errors=%s\\n" "${#HX_LOADED[@]}" '
        '"${#HX_ERRORS[@]}"\n'
        '[ "${#HX_ERRORS[@]}" -gt 0 ] && printf "%s\\n" "${HX_ERRORS[@]}"\n'
        'true\n')

# Management commands every install answers. Each must return 0 and must
# not crash; their output is theirs to format.
FRAMEWORK_CMDS = ["hxp list", "hxp catalog", "hxp doctor", "conf list",
                  "conf doctor", "help_conf", "conf reload"]

# The regression this file was written for, in both spellings: the exact
# idiom colored-man-pages uses, in the dialect the rc loads in, and the
# wrapped command itself when the machine has `man`.
IDIOM = ('e=(); e+=( PAGER="${PAGER:-less}" ); e+=( GROFF_NO_SGR=1 )\n'
         'command env "${e[@]}" sh -c \'echo idiom=$GROFF_NO_SGR\'\n'
         'for x in A=1 B=2; do printf "for=%s\\n" "$x"; done\n')

# Per-plugin smoke: name -> list of (label, needs, script, want). Runs only
# when that plugin directory exists in the sandbox, from a scratch cwd that
# is a fresh git repo. Everything here is read-only and offline: nothing
# that commits, pushes, deletes, changes directory for the user, talks to
# a docker daemon or to the network. A command that needs an external the
# machine lacks is skipped, by name.
PLUGIN_SMOKE = {
    "omz-colored-man-pages": [
        ("man --version", "man",
         "man --version 2>&1", b"man"),
    ],
}


def loaded_shell(script):
    return LOAD.replace('printf "loaded', ': "loaded') + script


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    print("--- shell: %s" % SHELL)
    mode = os.path.join(os.path.dirname(SHELL), ".mode")
    if os.path.exists(mode):
        print("--- build: %s" % open(mode).read().strip())
    home = tempfile.mkdtemp(prefix="hxp-sandbox-")
    try:
        origin = build_sandbox(home)
        if not origin:
            print("\nSKIP: no framework (no HXP_SRC, no ~/.polish, "
                  "nothing cached%s)" % (", offline" if OFFLINE else
                                         " and no network"))
            return 0
        fwdir = os.path.join(home, layout(home)[0])
        print("--- framework: %s (%s)" % (origin, os.path.basename(fwdir)))
        plugdir = os.path.join(fwdir, "plugins")
        plugins = sorted(d for d in os.listdir(plugdir)
                         if os.path.isfile(os.path.join(plugdir, d,
                                                        "plugin.hsh")))
        print("--- plugins: %s\n" % " ".join(plugins))

        # 1. The whole configuration loads: no crash, no recorded error.
        rc, out, err = run(home, LOAD)
        bad = crash_words(err, rc)
        check("load/no-crash", not bad, "%s; err=%r" % (bad, err[:300]))
        check("load/exit-0", rc == 0, "rc=%d err=%r" % (rc, err[:300]))
        check("load/no-recorded-errors", b"errors=0" in out,
              "out=%r" % out[-400:])

        # 2. The framework's own suite, against the sandbox copy.
        suite = os.path.join(fwdir, "test", "run.hsh")
        if os.path.isfile(suite):
            rc, out, err = run(home, "", argv=[suite], timeout=600)
            bad = crash_words(err, rc)
            check("own-suite/no-crash", not bad,
                  "%s; err=%r" % (bad, err[:300]))
            check("own-suite/passes", rc == 0 and b"passed" in out,
                  "rc=%d tail=%r" % (rc, out[-600:]))

        # 3. Management commands.
        for cmd in FRAMEWORK_CMDS:
            rc, out, err = run(home, loaded_shell(cmd + "\n"))
            bad = crash_words(err, rc)
            check("cmd/%s/no-crash" % cmd.replace(" ", "-"), not bad,
                  "%s; err=%r" % (bad, err[:300]))
            check("cmd/%s/exit-0" % cmd.replace(" ", "-"), rc == 0,
                  "rc=%d err=%r" % (rc, err[:300]))
        for name in plugins:
            rc, out, err = run(home, loaded_shell("hxp info %s\n" % name))
            check("info/%s" % name, rc == 0 and not crash_words(err, rc),
                  "rc=%d err=%r" % (rc, err[:200]))

        # 4. The regression idiom, inside the loaded configuration.
        rc, out, err = run(home, loaded_shell(IDIOM))
        bad = crash_words(err, rc)
        check("idiom/no-crash", not bad, "%s; err=%r" % (bad, err[:300]))
        check("idiom/array-element-shaped-like-an-assignment",
              b"idiom=1" in out and b"for=A=1" in out and b"for=B=2" in out,
              "rc=%d out=%r err=%r" % (rc, out[-200:], err[:200]))

        # 5. Use what the plugins define.
        work = os.path.join(home, "work")
        os.makedirs(work)
        subprocess.run(["git", "init", "-q", work], capture_output=True)
        for name in plugins:
            for label, needs, script, want in PLUGIN_SMOKE.get(name, []):
                if needs and not shutil.which(needs):
                    print("skip %s/%s (no %s on PATH)" % (name, label, needs))
                    continue
                rc, out, err = run(home, loaded_shell(script + "\n"),
                                   cwd=work)
                bad = crash_words(err, rc)
                check("%s/%s/no-crash" % (name, label), not bad,
                      "%s; err=%r" % (bad, err[:300]))
                check("%s/%s/runs" % (name, label),
                      rc == 0 and want in out,
                      "rc=%d out=%r err=%r" % (rc, out[-200:], err[:200]))
    finally:
        shutil.rmtree(home, ignore_errors=True)
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

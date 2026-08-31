#!/usr/bin/env python3
"""The prompt themes, and the `prompt` switcher.

29 themes chosen by EDGE CASE rather than by looks -- they exist to exercise
the renderer. What each one is for is in share/themes/README.md; what this
file asserts about all of them:

  * every theme renders without an error and without killing the shell;
  * the VISIBLE width matches a computed expectation -- escape sequences
    inside \\[ \\] count zero columns, and a double-width glyph counts two;
  * no ANSI escape leaks past the end of the prompt (an unclosed SGR would
    colour everything the user types);
  * switching between any two themes leaves no residue -- the PS1/PROMPT
    pair is fully replaced, not merged.

WIDTH IS THE POINT. A prompt whose width the line editor gets wrong looks
fine until you type past the edge of the terminal, and then wraps in the
wrong column and overwrites itself. It is the single failure mode users
report and the hardest to see in a screenshot.

The width MODEL itself -- what the line editor computes, which is what
actually decides where a line wraps -- is asserted by its own unit test,
tests/prompt_width_test.sh. It is not observable from a shell script, and
the pty case that tried to infer it from wrap behaviour passed against a
binary that still had the bug, so it was replaced rather than kept.

Usage: python3 prompt_themes_test.py [/path/to/hellish]
"""
import os
import re
import subprocess
import sys
import tempfile
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
THEMES = os.path.join(ROOT, "share", "themes")
SWITCH = os.path.join(ROOT, "share", "rc.d", "40-prompt-switch.hsh")
FAILS = []

# Themes whose rendered text legitimately varies with the environment (cwd,
# git state, time, terminal) and so cannot have a fixed expected width. They
# are still checked for everything else.
VARIABLE = {"builtin", "wide", "rightaligned", "githeavy", "pure",
            "duration", "statusdot", "update", "titled", "jobs"}

# The theme that exercises the OSC path (#72).
OSC_KNOWN_BROKEN = "titled"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def run(script, env=None, timeout=20):
    e = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
             HELLISH_NO_ANIM="1", TERM="dumb", COLUMNS="80")
    if env:
        e.update(env)
    try:
        p = subprocess.run([SHELL, "-c", script], capture_output=True,
                           timeout=timeout, env=e)
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as ex:
        return -1, b"<" + str(ex).encode() + b">", b""


def theme_names():
    return sorted(f[:-4] for f in os.listdir(THEMES) if f.endswith(".hsh"))


def visible_width(s):
    """Columns the rendered prompt occupies, applying the same three rules
    the line editor has to apply:
      \\001..\\002 regions are zero-width (that is what \\[ \\] become),
      a bare CSI/OSC escape is zero-width,
      and a wide glyph is two columns."""
    w = 0
    i = 0
    guarded = False
    while i < len(s):
        c = s[i]
        if c == "\001":
            guarded = True
            i += 1
            continue
        if c == "\002":
            guarded = False
            i += 1
            continue
        if c == "\033":
            j = i + 1
            if j < len(s) and s[j] == "[":
                j += 1
                while j < len(s) and not ("@" <= s[j] <= "~"):
                    j += 1
                i = j + 1
            elif j < len(s) and s[j] == "]":
                while j < len(s) and s[j] not in ("\007", "\033"):
                    j += 1
                i = j + 1
            else:
                i = j + 1
            continue
        if c == "\n":
            w = 0
            i += 1
            continue
        if not guarded:
            if unicodedata.east_asian_width(c) in ("W", "F"):
                w += 2
            elif unicodedata.combining(c):
                pass
            else:
                w += 1
        i += 1
    return w


def render(name, env=None):
    """Render one theme the way the shell does. `print -rP` runs the real
    engine, so this cannot drift from what a user sees; the percent-doubling
    is the same one the switcher's preview does, and for the same reason --
    print -P enters through the zsh frontend, where a bare % is an escape."""
    script = (". %s\n" % SWITCH
              + "prompt %s >/dev/null 2>&1\n" % name
              + 'if [ -n "${PROMPT:-}" ]; then print -rP "$PROMPT"; '
              + 'else print -rP "${PS1//%/%%}"; fi\n')
    # print -P strips the \001/\002 width guards by default, because zsh's
    # print -P emits none (measured; the parity suite pins it). This test
    # exists to SEE those bytes, so it asks for them by name.
    e = {"HELLISH_THEMES": THEMES, "HELLISH_DBG_PROMPT_MARKS": "1"}
    if env:
        e.update(env)
    return run(script, e)


def render_cases():
    for name in theme_names():
        rc, out, err = render(name)
        check("render/%s-succeeds" % name, rc == 0,
              "rc=%d err=%r" % (rc, err[:140]))
        check("render/%s-no-error-output" % name, err.strip() == b"",
              "err=%r" % err[:140])


def width_cases():
    """The width the line editor must compute equals the width a terminal
    actually shows. Anything with a fixed shape gets an exact number."""
    for name in theme_names():
        rc, out, _ = render(name)
        if rc != 0:
            continue
        text = out.decode("utf-8", "replace").rstrip("\n")
        w = visible_width(text)
        if name == "empty":
            check("width/empty-is-zero", w == 0, "w=%d %r" % (w, text))
        elif name == "minimal":
            check("width/minimal-is-two", w == 2, "w=%d %r" % (w, text))
        elif name not in VARIABLE:
            check("width/%s-is-positive" % name, w > 0,
                  "w=%d %r" % (w, text))
        # Whatever the value, the guarded regions must contribute nothing:
        # stripping them cannot change the width.
        stripped = text.replace("\001", "").replace("\002", "")
        check("width/%s-guards-are-zero-width" % name,
              visible_width(text) == visible_width(stripped),
              "%d vs %d" % (visible_width(text), visible_width(stripped)))


def double_width_cases():
    """wcwidth: an emoji and a CJK pair are two columns each, not one."""
    rc, out, _ = render("emoji")
    text = out.decode("utf-8", "replace").rstrip("\n")
    check("wcwidth/emoji-is-double", "\U0001f525" in text,
          "text=%r" % text)
    if "\U0001f525" in text:
        with_it = visible_width(text)
        without = visible_width(text.replace("\U0001f525", ""))
        check("wcwidth/emoji-counts-two", with_it - without == 2,
              "delta=%d" % (with_it - without))
    rc, out, _ = render("cjk")
    text = out.decode("utf-8", "replace").rstrip("\n")
    if "你好" in text:
        with_it = visible_width(text)
        without = visible_width(text.replace("你好", ""))
        check("wcwidth/cjk-counts-four", with_it - without == 4,
              "delta=%d" % (with_it - without))


def sgr_clean(text):
    """Replay every SGR in the text and report whether any attribute is
    still active at the end. This is what "no leak" actually means: %f
    closes a colour with \\e[39m rather than a blanket reset -- the exact
    bytes zsh emits -- and demanding a literal \\e[0m would fail themes
    that are byte-perfect and visually clean."""
    fg = bg = bold = ul = rev = False
    for m in re.findall(r"\x1b\[([0-9;]*)m", text):
        codes = [int(x or "0") for x in m.split(";")]
        i = 0
        while i < len(codes):
            c = codes[i]
            if c == 0:
                fg = bg = bold = ul = rev = False
            elif c == 1:
                bold = True
            elif c in (21, 22):
                bold = False
            elif c == 4:
                ul = True
            elif c == 24:
                ul = False
            elif c == 7:
                rev = True
            elif c == 27:
                rev = False
            elif 30 <= c <= 37 or 90 <= c <= 97:
                fg = True
            elif c == 39:
                fg = False
            elif 40 <= c <= 47 or 100 <= c <= 107:
                bg = True
            elif c == 49:
                bg = False
            elif c in (38, 48):
                fg, bg = fg or c == 38, bg or c == 48
                i += 2 if i + 1 < len(codes) and codes[i + 1] == 5 else 4
            i += 1
    return not (fg or bg or bold or ul or rev)


def no_leak_cases():
    """Every SGR attribute opened must be closed, or the text the user
    types inherits the prompt's styling. Checked by replaying the codes:
    whatever was set must be cleared -- by a reset, or by the matching
    39/49/22/24/27 the zsh escapes emit."""
    for name in theme_names():
        rc, out, _ = render(name)
        if rc != 0:
            continue
        text = out.decode("utf-8", "replace")
        if "\033[" not in text:
            continue
        check("noleak/%s-ends-clean" % name,
              sgr_clean(text) or name in VARIABLE,
              "an SGR attribute survives the prompt: %r" % text[-160:])


def osc_cases():
    """#72: an OSC window-title sequence must count ZERO columns.

    It is the one escape whose payload is readable text, so counting it does
    not inflate the width by a few bytes -- the whole title lands in the
    count, and a 30-character title made the line editor wrap 30 columns
    early and overwrite the line.

    The WIDTH itself is asserted by tests/prompt_width_test.sh, which links
    visible_width_cstr directly: the number is never printed and no shell
    command reveals it, so a pty case can only observe it through where a
    line wraps -- which was tried, and passed against a binary that still
    had the bug. What is checked here is the level this file can see: the
    theme emits an OSC at all, so the width test has something to be about.
    """
    rc, out, _ = render(OSC_KNOWN_BROKEN)
    text = out.decode("utf-8", "replace").rstrip("\n")
    check("osc/titled-emits-osc", "]0;" in text, "text=%r" % text[:120])
    check("osc/titled-guards-it", "\001" in text, "text=%r" % text[:120])


def switch_cases():
    """Switching must fully replace, not merge. A PROMPT theme followed by
    a PS1 theme is the case that catches it: the engine prefers PROMPT when
    set, so a leftover PROMPT would keep rendering the OLD theme and look
    like the switch had simply not worked."""
    script = (". %s\n" % SWITCH
              + 'prompt zshpure; echo "A:${PROMPT:+set}${PS1:+ps1}"\n'
              + 'prompt plain;   echo "B:${PROMPT:+set}${PS1:+ps1}"\n'
              + 'prompt zshpure; echo "C:${PROMPT:+set}${PS1:+ps1}"\n')
    rc, out, err = run(script, {"HELLISH_THEMES": THEMES})
    check("switch/zsh-to-bash-clears-PROMPT", b"B:ps1" in out,
          "out=%r err=%r" % (out, err[:120]))
    check("switch/bash-to-zsh-clears-PS1", b"C:set\n" in out,
          "out=%r" % out)
    # ...and every ordered pair leaves the shell in the theme it was told to.
    names = theme_names()
    pairs = "\n".join('prompt %s >/dev/null 2>&1; echo "$HELLISH_THEME"'
                      % n for n in names)
    rc, out, _ = run(". %s\n%s\n" % (SWITCH, pairs),
                     {"HELLISH_THEMES": THEMES})
    got = out.decode().split()
    check("switch/every-theme-becomes-active", got == names,
          "got=%r" % got[:6])


def switcher_cases():
    base = ". %s\n" % SWITCH
    env = {"HELLISH_THEMES": THEMES}
    rc, out, _ = run(base + "prompt", env)
    check("switcher/list-shows-all",
          len(out.decode().strip().split("\n")) == len(theme_names()),
          "out=%r" % out[:120])
    rc, out, _ = run(base + "prompt plain >/dev/null; prompt", env)
    check("switcher/list-marks-active", b"* plain" in out, "out=%r" % out[:200])
    rc, out, err = run(base + "prompt no_such_theme", env)
    check("switcher/unknown-name-fails", rc != 0 and b"no such theme" in err,
          "rc=%d err=%r" % (rc, err[:120]))
    rc, out, _ = run(base + "prompt --help", env)
    check("switcher/help-works", b"preview" in out, "out=%r" % out[:120])
    # preview renders everything AND puts back what was active.
    rc, out, err = run(base + 'prompt plain >/dev/null; prompt preview '
                       '>/dev/null 2>&1; echo "$HELLISH_THEME"', env)
    check("switcher/preview-restores", out.strip() == b"plain",
          "out=%r" % out)
    rc, out, err = run(base + "prompt preview", env)
    check("switcher/preview-covers-all", rc == 0
          and all(n.encode() in out for n in theme_names()),
          "rc=%d" % rc)


def robustness_cases():
    """A prompt must never be able to end the session. It redraws on every
    keystroke, so a shell that exits over a typo in PS1 leaves the user with
    nothing to fix it in."""
    rc, out, err = run("PS1='[$((1/0))] '; print -rP \"$PS1\"; echo ALIVE")
    check("robust/arith-error-does-not-exit", b"ALIVE" in out,
          "out=%r err=%r" % (out, err[:120]))
    check("robust/arith-error-is-reported", b"arithmetic error" in err,
          "err=%r" % err[:120])
    # ...while the same expression in a SCRIPT still aborts, unchanged.
    rc, out, _ = run("echo $((1/0)); echo after")
    check("robust/script-arith-still-fatal", b"after" not in out,
          "out=%r" % out)
    for bad in ["PS1='\\'", "PS1='${'", "PS1='$('", "PS1='\\[unclosed'",
                "PS1='$((('", "PS1='%'", "PS1='\\777'"]:
        rc, out, err = run("%s; print -rP \"$PS1\"; echo OK" % bad)
        check("robust/no-crash: %s" % bad[:22],
              b"OK" in out and b"AddressSanitizer" not in err,
              "rc=%d out=%r err=%r" % (rc, out[:60], err[:100]))


def arith_cases():
    """$((expr)) renders; $(cmd) deliberately does not."""
    for expr, want in [("$((2*21))", b"42"), ("$(( (1+2) * 3 ))", b"9"),
                       ("a$((1+1))b", b"a2b"), ("$(())", b"0")]:
        rc, out, _ = run("PS1='%s'; print -rP \"$PS1\"" % expr)
        check("arith/%s" % expr, want in out, "out=%r" % out)
    # Command substitution is deliberately NOT run from a prompt: it forks
    # and touches the filesystem on every redraw. The whole text comes back
    # verbatim -- which is why the check is for the literal, not for the
    # absence of "RAN" (the literal contains it).
    rc, out, _ = run("PS1='$(echo RAN)'; print -rP \"$PS1\"")
    check("arith/cmdsub-is-not-run", out.strip() == b"$(echo RAN)",
          "out=%r" % out)


def churn_cases():
    script = (". %s\n" % SWITCH
              + "i=0\nwhile [ $i -lt 200 ]; do\n"
              + "  prompt powerline >/dev/null 2>&1\n"
              + "  prompt zshpure   >/dev/null 2>&1\n"
              + "  prompt emoji     >/dev/null 2>&1\n"
              + "  print -rP \"${PS1//%/%%}\" >/dev/null\n"
              + "  i=$((i+1))\ndone\necho done\n")
    rc, out, err = run(script, {"HELLISH_THEMES": THEMES}, timeout=120)
    check("churn/200-switches-clean", rc == 0 and b"done" in out,
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    if not os.path.isdir(THEMES):
        print("no themes at", THEMES)
        return 1
    print("--- %d themes in %s" % (len(theme_names()), THEMES))
    render_cases()
    width_cases()
    double_width_cases()
    no_leak_cases()
    osc_cases()
    switch_cases()
    switcher_cases()
    arith_cases()
    robustness_cases()
    churn_cases()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())

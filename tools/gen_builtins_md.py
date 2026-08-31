#!/usr/bin/env python3
"""Generate wiki/builtins/index.md from the shell's OWN help system.

The single source of truth is src/builtins/help_data*.c, which `make
help-test` already forces to cover every name in the dispatch table -- so a
reference generated from `help` output can no more drift from the shell than
the help builtin itself can. Hand-written pages rot; this one is re-emitted
by `make docs-builtins` and committed, so the docs site needs no compiler.

Usage: python3 tools/gen_builtins_md.py [/path/to/hellish]
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
OUT = os.path.join(ROOT, "wiki", "builtins", "index.md")
ANSI = re.compile(r"\x1b\[[0-9;]*m")

# Deep-dive pages that exist beside the generated index; the name links there
# instead of only carrying its one-liner.
DIVES = {"cd": "cd.md"}


def run(script):
    env = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
               HELLISH_NO_ANIM="1")
    p = subprocess.run([SHELL, "-c", script], capture_output=True, text=True,
                       env=env, timeout=30)
    return ANSI.sub("", p.stdout)


def topic(name):
    """(group, synopsis, description) for one `help NAME` entry."""
    lines = [l for l in run("help '%s'" % name).splitlines() if l.strip()]
    if not lines:
        return None
    m = re.match(r"^(\S+)\s+\((\w+)\)", lines[0])
    group = m.group(2) if m else "misc"
    syn = lines[1].strip() if len(lines) > 1 else ""
    desc = " ".join(l.strip() for l in lines[2:])
    return group, syn, desc


def main():
    names = [n for n in run("compgen -A builtin").split() if n]
    # group order and the syntax topics come from the grouped listing
    order, syntax = [], []
    grp = None
    for line in run("help").splitlines():
        if re.match(r"^\w+$", line.strip()) and not line.startswith(" "):
            grp = line.strip()
            if grp not in order:
                order.append(grp)
        elif grp == "syntax" and line.startswith("  "):
            syntax.append(line.split()[0])

    entries = {}
    for n in names + syntax:
        t = topic(n)
        if t:
            entries.setdefault(t[0], []).append((n, t[1], t[2]))
    for g in entries:
        if g not in order:
            order.append(g)

    with open(OUT, "w") as f:
        f.write(
            "# Builtins — the reference\n\n"
            "> Every name built into the shell, grouped the way `help` groups\n"
            "> them, with the same synopses `help NAME` prints — because this\n"
            "> page IS `help` output: regenerate it with `make docs-builtins`\n"
            "> (tools/gen_builtins_md.py), never edit it by hand. The help\n"
            "> table itself is test-enforced against the dispatch table, so\n"
            "> neither this page nor `help` can drift from what actually\n"
            "> runs. %d builtins; anything else on `$PATH` works as usual —\n"
            "> `type NAME` says which is which.\n\n" % len(names))
        for g in order:
            if g not in entries:
                continue
            f.write("## %s\n\n" % g)
            if g == "syntax":
                f.write("Not builtins — the grammar `help` also explains, "
                        "kept here for the same one-stop reason.\n\n")
            if g == "zsh":
                f.write("Only reachable when the dialect is armed "
                        "(`set -o zsh`, `emulate zsh`, or sourcing a `.zsh` "
                        "file) — see the "
                        "[zsh dialect](../architecture.md#the-zsh-dialect).\n\n")
            for n, syn, desc in entries[g]:
                shown = "[`%s`](%s)" % (n, DIVES[n]) if n in DIVES \
                    else "`%s`" % n
                f.write("**%s** — `%s`\n" % (shown, syn) if syn else
                        "**%s**\n" % shown)
                if desc:
                    f.write("<br>%s\n" % desc)
                f.write("\n")
    print("wrote %s (%d names, %d groups)"
          % (OUT, len(names) + len(syntax), len(order)))


main()

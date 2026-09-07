#!/usr/bin/env python3
"""Who interpreted what, from an execlog (tests/tools/execlog*.c) log.

    execlog_classify.py <log> <out-prefix> <corpus-dir> <launcher>

One line per exec in <log>: pid, caller image, path, shebang, argv
(\\x1f-joined). The interpreter of a file is what its shebang says (env X ->
X), else the file itself. A shell is an OFFENDER when the corpus's own code
started it: a file under <corpus-dir> interpreted by bash/sh/dash/zsh, or
hellish/make/<launcher> exec'ing one by name. A third-party program that is
itself a sh script (VBoxManage, xdg-settings, code) or spawns one is FOREIGN:
reported, not charged to the corpus.

Writes <out-prefix>.scripts   corpus file -> interpreters that ran it
       <out-prefix>.offenders shells the corpus started (empty is the goal)
       <out-prefix>.foreign   third-party sh
       <out-prefix>.interp    every image exec'd, with a count
"""
import os
import sys

SHELLS = {"bash", "sh", "dash", "zsh", "ksh", "mksh", "ash"}


def main():
    log, out, b2r, launcher = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
    b2r = os.path.abspath(b2r)
    ours = {"hellish", "hellish.real", "make", os.path.basename(launcher)}
    scripts, offenders, foreign, interp = {}, [], [], {}

    def under(p):
        return os.path.abspath(p).startswith(b2r + os.sep)

    for raw in open(log, encoding="utf-8", errors="replace"):
        parts = raw.rstrip("\n").split("\t", 4)
        if len(parts) < 5:
            continue
        pid, caller, path, sb, argv = parts
        argv = argv.split("\x1f")
        words = sb.split() if sb != "-" else []
        if words:
            ip = words[1] if os.path.basename(words[0]) == "env" and len(words) > 1 else words[0]
        else:
            ip = path
        name = os.path.basename(ip)
        interp[name] = interp.get(name, 0) + 1
        shown = os.path.relpath(path, b2r) if under(path) else path
        if under(path) and (path.endswith(".sh") or words):
            scripts.setdefault(shown, set()).add(name)
        # `<shell> script.sh args`: the script is argv's first non-option word
        # that is a file of the corpus; `<shell> -c '...'` is a recipe line.
        if os.path.basename(path) in SHELLS | {"hellish", "hellish.real"}:
            rest = argv[1:]
            if rest and rest[0] == "-c":
                scripts.setdefault("<recipe line>", set()).add(os.path.basename(path))
            else:
                for a in rest:
                    if a.startswith("-"):
                        continue
                    full = a if os.path.isabs(a) else os.path.join(b2r, a)
                    if under(full) and os.path.isfile(full):
                        scripts.setdefault(os.path.relpath(full, b2r), set()).add(os.path.basename(path))
                    break
        if name in SHELLS:
            if under(path) or (os.path.basename(path) in SHELLS and os.path.basename(caller) in ours):
                offenders.append((shown, os.path.basename(caller), " ".join(argv)[:140]))
            else:
                foreign.append((shown, os.path.basename(caller), " ".join(argv)[:100]))
    with open(out + ".scripts", "w") as f:
        for p in sorted(scripts):
            f.write("%s\t%s\n" % (p, ",".join(sorted(scripts[p]))))
    with open(out + ".offenders", "w") as f:
        for p, c, a in offenders:
            f.write("%s\t(from %s)\t%s\n" % (p, c, a))
    with open(out + ".foreign", "w") as f:
        for p, c, a in sorted(set(foreign)):
            f.write("%s\t(from %s)\t%s\n" % (p, c, a))
    with open(out + ".interp", "w") as f:
        for k in sorted(interp):
            f.write("%s\t%d\n" % (k, interp[k]))


if __name__ == "__main__":
    main()

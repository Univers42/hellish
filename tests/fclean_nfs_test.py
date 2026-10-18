#!/usr/bin/env python3
"""`make fclean` must not fail on a file NFS keeps for a running process.

On NFS, deleting a file that a process still has open leaves it behind as
.nfsXXXX until that process exits, and removing it fails with "Device or
resource busy". With a shell from build/bin still running, `make re` used to
stop in fclean on exactly that file, with nothing rebuilt.

This runs the Makefile's own cleanup recipe (cut out of the Makefile, so the
test cannot drift from it) in a scratch directory next to the repo:
  - only a held-open .nfs file left: fclean succeeds and says what it kept;
  - a real leftover (an unwritable directory): fclean still fails.
Where the repo is not on NFS no .nfs file ever appears; that is a SKIP, not a
pass. argv[1] (the shell, as for every tests/*.py) is not used.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        FAILS.append(name)
        print("     " + detail)


def recipe():
    """The cleanup block of the fclean target, from `@if ! rm -rf build` to
    its closing `fi`."""
    out, inside = [], False
    with open(os.path.join(ROOT, "Makefile")) as f:
        for line in f:
            if line.startswith("\t@if ! rm -rf build"):
                inside = True
            if inside:
                out.append(line)
                if line == "\tfi\n":
                    return "".join(out)
    return None


def fclean(d):
    return subprocess.run(["make", "-s", "-C", d, "fclean"],
                          capture_output=True, text=True, timeout=30)


def main():
    body = recipe()
    check("recipe/found-in-Makefile", body is not None)
    if body is None:
        return 1
    d = tempfile.mkdtemp(prefix=".fclean-nfs-", dir=ROOT)
    holder = None
    try:
        with open(os.path.join(d, "Makefile"), "w") as f:
            f.write("fclean:\n" + body)
        os.makedirs(os.path.join(d, "build", "bin"))
        held = os.path.join(d, "build", "bin", "held")
        with open(held, "w") as f:
            f.write("x")
        holder = subprocess.Popen(["sh", "-c", "exec 3<\"$1\"; exec sleep 30",
                                   "sh", held])
        time.sleep(0.3)
        os.unlink(held)
        if not any(n.startswith(".nfs")
                   for n in os.listdir(os.path.join(d, "build", "bin"))):
            print("SKIP not on NFS: no .nfs file is left for an open file")
            return 0
        r = fclean(d)
        check("nfs-only/fclean-succeeds", r.returncode == 0, r.stderr)
        check("nfs-only/says-what-it-kept", "kept build/bin/.nfs" in r.stderr,
              r.stderr)
        locked = os.path.join(d, "build", "locked")
        os.makedirs(locked)
        open(os.path.join(locked, "f"), "w").close()
        os.chmod(locked, 0o555)
        r = fclean(d)
        os.chmod(locked, 0o755)
        check("real-leftover/fclean-still-fails", r.returncode != 0, r.stderr)
    finally:
        if holder:
            holder.kill()
            holder.wait()
        # NFS drops the .nfs file only once the holder is gone; retry.
        for _ in range(20):
            shutil.rmtree(d, ignore_errors=True)
            if not os.path.exists(d):
                break
            time.sleep(0.25)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())

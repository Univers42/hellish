#!/usr/bin/env python3
"""Regression for issue #164: a curl-fed installer must not lose its PATH hint.

The real installer prints a long message through printf '%s'. Put the
important text across the old 4095-byte output boundary: before the fix the
line ended at "Manually", hiding the rest of the instruction.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else
                        os.path.join(ROOT, "build", "bin", "hellish"))
FIXTURE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       ".mini_install.sh")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")


def main():
    with open(FIXTURE, "rb") as source:
        script = source.read()
    proc = subprocess.run([SHELL], input=script, capture_output=True,
                          env=ENV, timeout=15)
    expected = b"Manually add this path to PATH"
    ok = proc.returncode == 0 and expected in proc.stdout
    print(("ok   " if ok else "FAIL ")
          + "curl-fed installer prints the complete PATH instruction")
    if not ok:
        print("     exit=%s, stdout tail=%r, stderr=%r" %
              (proc.returncode, proc.stdout[-100:], proc.stderr[-300:]))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

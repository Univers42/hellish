#!/bin/bash
# Run an external, third-party 42 "minishell tester" (the geoman-style
# black-box testers many 42 students use) against the built hellish binary.
#
# These testers are an INDEPENDENT sanity check that complements the in-tree
# harnesses -- which are already broader and stricter:
#   make test         ~2870 golden-diff cases vs bash --posix (tests/tester)
#   make conformance  Oils spec + mksh check.t vs bash --posix AND dash
#
# The tester repo is configurable so you can point at whichever one you use:
#   make geoman                                  # default tester below
#   make geoman GEOMAN_URL=https://github.com/you/your_tester
#
# We clone once into bench/suites/geoman (gitignored) and re-run in place.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/bin/hellish"
DEST="$ROOT/bench/suites/geoman"
URL="${GEOMAN_URL:-https://github.com/zstenger93/42_minishell_tester}"

if [ ! -x "$BIN" ]; then
    echo "error: build hellish first (make all OPT=1, or make all)" >&2
    exit 2
fi

if [ ! -d "$DEST" ]; then
    echo ">> cloning external tester: $URL" >&2
    if ! git clone --depth 1 --quiet "$URL" "$DEST" 2>/dev/null; then
        cat >&2 <<EOF
!! Could not clone the external tester from:
!!     $URL
!! Point make geoman at the tester you use:
!!     make geoman GEOMAN_URL=<repo-url>
!! Meanwhile the in-tree harnesses cover the same ground, and more:
!!     make test         (~2870 cases vs bash --posix)
!!     make conformance  (Oils spec + mksh check.t vs bash + dash)
EOF
        exit 1
    fi
fi

# Adapt the cloned tester to run IN PLACE against hellish.  These testers
# invoke a binary named `minishell` in a fixed directory, so we:
#   - symlink ./minishell -> the hellish binary (conventional target name),
#   - point the tester's RUNDIR/MINISHELL_PATH at the clone dir (many hardcode
#     $HOME/42_minishell_tester), so it finds its own cases and our binary,
#   - run the "mandatory" mode by default (override with GEOMAN_MODE=...).
# hellish runs in default mode here (not --posix): geoman-style testers diff
# against plain bash, a looser check than `make conformance`.
cd "$DEST" || exit 1
runner=""
for cand in tester.sh minishell_tester.sh tester run.sh; do
    [ -f "$cand" ] && { runner="$cand"; break; }
done
if [ -z "$runner" ]; then
    echo "!! no known entry point (tester.sh/tester/run.sh) in $DEST" >&2
    echo "!! run it manually against: $BIN" >&2
    ls -1 >&2
    exit 1
fi

ln -sf "$BIN" ./minishell 2>/dev/null || true
# Repoint any hardcoded RUNDIR at the actual clone location.
sed -i "s|^RUNDIR=.*|RUNDIR=$DEST|" "$runner" 2>/dev/null || true
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1
echo ">> running $runner (${GEOMAN_MODE:-m}) against $BIN" >&2
chmod +x "$runner" 2>/dev/null || true
RUNDIR="$DEST" MINISHELL_PATH="$DEST" bash "$runner" "${GEOMAN_MODE:-m}"

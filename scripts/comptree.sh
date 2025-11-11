#!/usr/bin/env bash
# Show a single unified diff of two branches in VSCode (temporary readonly file).
# Usage: ./scripts/show-branch-diff.sh <branchA> <branchB> [--fetch] [--three-dot] [--wait]
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 branchA branchB [--fetch] [--three-dot] [--wait]
  --fetch      : run 'git fetch --all' before generating diff
  --three-dot  : use 'branchA...branchB' (compare vs common ancestor) instead of 'branchA..branchB'
  --wait       : open VSCode and wait; temp diff file will be removed after VSCode exits
EOF
  exit 1
}

if [ $# -lt 2 ]; then usage; fi

BR_A="$1"; BR_B="$2"; shift 2
FETCH=0; THREE_DOT=0; WAIT=0
while [ $# -gt 0 ]; do
  case "$1" in
    --fetch) FETCH=1;;
    --three-dot) THREE_DOT=1;;
    --wait) WAIT=1;;
    *) usage;;
  esac
  shift
done

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

# Corrected: use a conditional test, not "$FETCH && ..."
if [ "$FETCH" -eq 1 ]; then
  git fetch --all --prune
fi

SEP=".."
if [ "$THREE_DOT" -eq 1 ]; then
  SEP="..."
fi

TMP=$(mktemp /tmp/git-branch-diff.XXXXXX.diff)
trap 'rm -f "$TMP" >/dev/null 2>&1 || true' EXIT

{
  printf "Repository: %s\n" "$REPO_ROOT"
  printf "Diff comparison: %s%s%s\n\n" "$BR_A" "$SEP" "$BR_B"
  printf "Command: git diff %s%s%s\n\n" "$BR_A" "$SEP" "$BR_B"
} > "$TMP"

git diff --no-ext-diff --stat --summary "$BR_A$SEP$BR_B" >> "$TMP" 2>/dev/null || true
printf "\n\n" >> "$TMP"
git diff --no-ext-diff "$BR_A$SEP$BR_B" >> "$TMP" 2>/dev/null || true

chmod a-w "$TMP" || true

if command -v code >/dev/null 2>&1; then
  if [ "$WAIT" -eq 1 ]; then
    code --reuse-window --wait "$TMP"
    rm -f "$TMP" || true
  else
    code --reuse-window "$TMP"
    echo "Opened diff at: $TMP (temporary file; remove manually if desired)"
  fi
else
  echo "VSCode CLI 'code' not found. Diff saved to: $TMP"
fi

exit 0
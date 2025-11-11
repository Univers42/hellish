#!/usr/bin/env bash
# Quick helper: create worktrees for two branches, open both in VSCode and optionally remove created worktrees after VSCode closes.
# Usage: ./scripts/compare-branches.sh <branchA> <branchB> [--fetch] [--rm-after-exit]
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 branchA branchB [--fetch] [--rm-after-exit]
  --fetch         : run 'git fetch --all' before creating worktrees
  --rm-after-exit : wait for VSCode to close and remove any temporary worktrees created by this script
EOF
  exit 1
}

if [ $# -lt 2 ]; then usage; fi

BR_A="$1"; BR_B="$2"; shift 2
FETCH=0; RM_AFTER=0
while [ $# -gt 0 ]; do
  case "$1" in
    --fetch) FETCH=1;;
    --rm-after-exit) RM_AFTER=1;;
    *) usage;;
  esac
  shift
done

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"
CUR_BRANCH="$(git branch --show-current || true)"

[ "$FETCH" -eq 1 ] && git fetch --all --prune

created_paths=()
paths_to_open=()

add_or_reuse() {
  local br="$1"
  # if branch is currently checked out in this repo, reuse repo root
  if [ "$br" = "$CUR_BRANCH" ]; then
    paths_to_open+=("$REPO_ROOT")
    return 0
  fi
  # try to add a temporary worktree
  tmpdir="$(mktemp -d /tmp/git-compare-${br//\//_}-XXXXXX)"
  if git worktree add --detach "$tmpdir" "$br" >/dev/null 2>&1; then
    created_paths+=("$tmpdir")
    paths_to_open+=("$tmpdir")
    return 0
  fi
  # if branch is already used by some worktree, reuse that path
  existing="$(git worktree list --porcelain | awk '/^worktree /{print $2} /^branch /{b=$2} b==("'"$br"'") {print prev; prev="";} {prev=$2}' 2>/dev/null || true)"
  # fallback: scan worktree list for matching branch name
  if git worktree list --porcelain 2>/dev/null | awk -v b="$br" '
    BEGIN{p=""}
    /^worktree /{wt=$2}
    /^branch /{ if ($2==b) print wt}
  ' | read -r wtpath; then
    paths_to_open+=("$wtpath")
    return 0
  fi
  # couldn't create or locate
  echo "[ERROR] Could not create or locate worktree for branch '$br'." >&2
  rm -rf "$tmpdir" || true
  exit 2
}

add_or_reuse "$BR_A"
add_or_reuse "$BR_B"

echo "[INFO] Opening VSCode for:"
for p in "${paths_to_open[@]}"; do echo "  $p"; done

# open VSCode in a new window and wait so we can cleanup after user closes if requested
if command -v code >/dev/null 2>&1; then
  if [ "$RM_AFTER" -eq 1 ]; then
    code --new-window --wait "${paths_to_open[@]}"
  else
    code -n "${paths_to_open[@]}"
  fi
else
  echo "[WARN] 'code' CLI not found. Open these paths manually in VSCode:"
  for p in "${paths_to_open[@]}"; do echo "  $p"; done
fi

# cleanup created worktrees if requested
if [ "$RM_AFTER" -eq 1 ] && [ "${#created_paths[@]}" -gt 0 ]; then
  echo "[INFO] Removing temporary worktrees..."
  for p in "${created_paths[@]}"; do
    # `git worktree remove` wants the path relative to repo or absolute; use absolute
    git worktree remove --force "$p" >/dev/null 2>&1 || true
    rm -rf "$p" >/dev/null 2>&1 || true
    echo "  removed $p"
  done
fi

exit 0
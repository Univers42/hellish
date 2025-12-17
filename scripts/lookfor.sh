#!/bin/bash
# ...existing code...
set -euo pipefail

dirs=(
  ./docs
  ./git
  ./github
  ./hellish
  ./vscode
  ./lab
  ./page
  ./scripts
  ./vscode
)

dryrun=false
toggle=false
yes=false

for arg in "$@"; do
  case "$arg" in
    --dry-run|-n) dryrun=true ;;
    --toggle|-t) toggle=true ;;
    --yes|-y) yes=true ;;
    --help|-h) printf "Usage: %s [--dry-run|-n] [--toggle|-t] [--yes|-y]\n" "$0"; exit 0 ;;
    *) printf 'Unknown option: %s\n' "$arg"; exit 2 ;;
  esac
done

prompt_yes() {
  local prompt="$1"
  if $yes; then
    return 0
  fi
  read -r -p "$prompt [y/N] " reply
  case "$reply" in
    [Yy]*) return 0 ;;
    *) return 1 ;;
  esac
}

for d in "${dirs[@]}"; do
  dir="${d%/}"   # remove trailing slash
  base="$(basename "$dir")"
  dirpath="$(dirname "$dir")"
  visible="$dirpath/$base"
  hidden="$dirpath/.$base"

  # Determine which exists
  exists_visible=false
  exists_hidden=false
  [[ -e "$visible" ]] && exists_visible=true
  [[ -e "$hidden" ]] && exists_hidden=true

  if ! $exists_visible && ! $exists_hidden; then
    printf 'Not found (neither visible nor hidden): %s\n' "$dir"
    continue
  fi

  # If both exist, skip to avoid clobbering
  if $exists_visible && $exists_hidden; then
    printf 'Both visible and hidden exist, skipping: %s -> %s and %s\n' "$dir" "$visible" "$hidden"
    continue
  fi

  # If visible exists -> hide it
  if $exists_visible; then
    if $dryrun; then
      printf 'Would rename (hide): %s -> %s\n' "$visible" "$hidden"
    else
      mv -- "$visible" "$hidden" && printf 'Renamed (hidden): %s -> %s\n' "$visible" "$hidden"
    fi
    continue
  fi

  # At this point only hidden exists
  # If toggle enabled, ask/auto-unhide; otherwise report already hidden
  if ! $toggle; then
    printf 'Already hidden: %s\n' "$hidden"
    continue
  fi

  # toggle was requested -> ask user (or auto-yes) to unhide
  if prompt_yes "Unhide $hidden -> $visible ?"; then
    if $dryrun; then
      printf 'Would rename (unhide): %s -> %s\n' "$hidden" "$visible"
    else
      mv -- "$hidden" "$visible" && printf 'Renamed (unhidden): %s -> %s\n' "$hidden" "$visible"
    fi
  else
    printf 'Skipped unhide: %s\n' "$hidden"
  fi
done
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
toggle_all=false
yes=false

for arg in "$@"; do
  case "$arg" in
    --dry-run|-n) dryrun=true ;;
    --toggle|-t) toggle=true ;;
    --toggle-all) toggle_all=true ;;
    --yes|-y) yes=true ;;
    --help|-h) printf "Usage: %s [--dry-run|-n] [--toggle|-t] [--toggle-all] [--yes|-y]\n" "$0"; exit 0 ;;
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

  # toggle-all requested: flip without prompting
  if $toggle_all; then
    if $exists_visible; then
      if $dryrun; then
        printf 'Would rename (hide): %s -> %s\n' "$visible" "$hidden"
      else
        mv -- "$visible" "$hidden" && printf 'Renamed (hidden): %s -> %s\n' "$visible" "$hidden"
      fi
    else
      if $dryrun; then
        printf 'Would rename (unhide): %s -> %s\n' "$hidden" "$visible"
      else
        mv -- "$hidden" "$visible" && printf 'Renamed (unhidden): %s -> %s\n' "$hidden" "$visible"
      fi
    fi
    continue
  fi

  # If --toggle: ask per-item and flip either way
  if $toggle; then
    if $exists_visible; then
      if prompt_yes "Hide $visible -> $hidden ?"; then
        if $dryrun; then
          printf 'Would rename (hide): %s -> %s\n' "$visible" "$hidden"
        else
          mv -- "$visible" "$hidden" && printf 'Renamed (hidden): %s -> %s\n' "$visible" "$hidden"
        fi
      else
        printf 'Skipped hide: %s\n' "$visible"
      fi
    else
      # exists_hidden must be true here
      if prompt_yes "Unhide $hidden -> $visible ?"; then
        if $dryrun; then
          printf 'Would rename (unhide): %s -> %s\n' "$hidden" "$visible"
        else
          mv -- "$hidden" "$visible" && printf 'Renamed (unhidden): %s -> %s\n' "$hidden" "$visible"
        fi
      else
        printf 'Skipped unhide: %s\n' "$hidden"
      fi
    fi
    continue
  fi

  # default behavior (no toggle flags): hide visible, report hidden
  if $exists_visible; then
    if $dryrun; then
      printf 'Would rename (hide): %s -> %s\n' "$visible" "$hidden"
    else
      mv -- "$visible" "$hidden" && printf 'Renamed (hidden): %s -> %s\n' "$visible" "$hidden"
    fi
    continue
  fi

  printf 'Already hidden: %s\n' "$hidden"
done
# ...existing code...
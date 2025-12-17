#!/usr/bin/env bash
# Safe automated repair for lib/libft submodule
# Creates a backup of lib/libft before attempting fixes.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
GIT_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -n "$GIT_ROOT" ]; then
	ROOT="$GIT_ROOT"
else
	ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

SUBMODULE_REL="lib/libft"
SUBMODULE="$ROOT/$SUBMODULE_REL"
MODULE_PATH="$ROOT/.git/modules/libft"
BACKUP="${SUBMODULE}.bak_$(date +%s)"

if [ ! -d "$SUBMODULE" ]; then
	printf "ERROR: submodule directory '%s' not found (resolved to '%s')\n" "$SUBMODULE_REL" "$SUBMODULE" >&2
	exit 1
fi

echo "Backing up '$SUBMODULE' -> '$BACKUP'"
mv "$SUBMODULE" "$BACKUP"

echo "Deinitializing submodule (if present)..."
git -C "$ROOT" submodule deinit -f -- "$SUBMODULE_REL" >/dev/null 2>&1 || true

echo "Removing possibly-broken modules metadata at '$MODULE_PATH'"
rm -rf "$MODULE_PATH" || true

echo "Attempting to reinitialize submodule..."
if git -C "$ROOT" submodule update --init --recursive "$SUBMODULE_REL"; then
	echo "Submodule reinitialized successfully."
else
	echo "git submodule update failed — attempting to re-add using .gitmodules URL..."
	URL="$(git -C "$ROOT" config -f "$ROOT/.gitmodules" --get "submodule.$SUBMODULE_REL.url" || true)"
	if [ -n "$URL" ]; then
		echo "Re-adding submodule from $URL"
		git -C "$ROOT" submodule add "$URL" "$SUBMODULE_REL"
		git -C "$ROOT" submodule update --init --recursive "$SUBMODULE_REL"
		echo "Submodule added and initialized."
	else
		echo "Could not find submodule URL in .gitmodules. Manual intervention required."
		echo "Suggested manual steps:"
		echo "  git submodule deinit -f -- $SUBMODULE_REL"
		echo "  rm -rf $ROOT/.git/modules/libft"
		echo "  git submodule update --init --recursive $SUBMODULE_REL"
		exit 2
	fi
fi

echo
echo "Repair finished. Backup of original content is at: $BACKUP"
echo "If you had local changes in the submodule, inspect and restore them as needed, e.g.:"
echo "  ls -la $BACKUP"
echo "  mv $BACKUP/<modified_files> $SUBMODULE/"
echo
echo "Verify status:"
echo "  git -C \"$ROOT\" submodule status"
echo "  git -C \"$SUBMODULE\" status || true"

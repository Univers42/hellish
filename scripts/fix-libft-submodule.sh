#!/usr/bin/env bash
# repair lib/libft submodule metadata (safe, makes backups)
# This script can be run from any directory; it will locate the repo root.
set -euo pipefail

# determine script dir and repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
GIT_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -n "$GIT_ROOT" ]; then
	ROOT="$GIT_ROOT"
else
	# fallback: assume the script is in <repo>/scripts/
	ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

SUBMODULE_REL="lib/libft"
SUBMODULE="$ROOT/$SUBMODULE_REL"
MODULE_PATH="$ROOT/.git/modules/libft"
BACKUP_SUFFIX=".bak_by_fixscript_$(date +%s)"

if [ ! -d "$SUBMODULE" ]; then
	echo "Submodule directory '$SUBMODULE_REL' not found (resolved to '$SUBMODULE'). Run from repo root or ensure submodule exists."
	exit 1
fi

echo "== Repairing submodule $SUBMODULE_REL (root: $ROOT) =="
# Show current .git inside submodule (file or dir)
if [ -e "$SUBMODULE/.git" ]; then
	if [ -d "$SUBMODULE/.git" ]; then
		echo "Detected $SUBMODULE/.git is a directory (contains git metadata)."
		echo "Will move its contents into $MODULE_PATH and replace with gitdir file."
		echo "Creating backup of $SUBMODULE -> ${SUBMODULE}${BACKUP_SUFFIX}"
		mv "$SUBMODULE" "${SUBMODULE}${BACKUP_SUFFIX}"
		mkdir -p "$SUBMODULE"
		# restore working tree files except .git from backup
		shopt -s dotglob
		for f in "${SUBMODULE}${BACKUP_SUFFIX}"/*; do
			name="$(basename "$f")"
			if [ "$name" = ".git" ]; then
				continue
			fi
			mv "$f" "$SUBMODULE/" || true
		done
		shopt -u dotglob
		# create modules dir and move .git contents
		mkdir -p "$(dirname "$MODULE_PATH")"
		echo "Moving git metadata to $MODULE_PATH"
		mkdir -p "$MODULE_PATH"
		# prefer rsync if available
		if command -v rsync >/dev/null 2>&1; then
			rsync -a "${SUBMODULE}${BACKUP_SUFFIX}/.git/" "$MODULE_PATH/"
		else
			cp -a "${SUBMODULE}${BACKUP_SUFFIX}/.git/." "$MODULE_PATH/" || true
		fi
		rm -rf "${SUBMODULE}${BACKUP_SUFFIX}/.git"
		printf "gitdir: ../.git/modules/libft\n" > "$SUBMODULE/.git"
		echo "Repair complete. Backup of original submodule at ${SUBMODULE}${BACKUP_SUFFIX}"
		exit 0
	elif [ -f "$SUBMODULE/.git" ]; then
		echo "Detected $SUBMODULE/.git is a file. Contents:"
		cat "$SUBMODULE/.git"
		# read target
		target=$(sed -n 's/^gitdir: *//p' "$SUBMODULE/.git" || true)
		if [ -z "$target" ]; then
			echo "Unknown .git file format. Manual fix required."
			exit 2
		fi
		# make target absolute if relative
		case "$target" in
			/*) target_abs="$target" ;;
			*) target_abs="$SUBMODULE/$target" ;;
		esac
		if [ -d "$target_abs" ]; then
			echo "Target directory $target_abs exists. Submodule metadata seems OK."
			exit 0
		else
			echo "Target $target_abs does not exist. Attempting to recover from embedded metadata..."
			# try to find any stray metadata under submodule backup locations
			if [ -d "${SUBMODULE}/.git/" ]; then
				echo "Found ${SUBMODULE}/.git/ directory unexpectedly. Moving to $MODULE_PATH"
				mkdir -p "$(dirname "$MODULE_PATH")"
				mkdir -p "$MODULE_PATH"
				if command -v rsync >/dev/null 2>&1; then
					rsync -a "${SUBMODULE}/.git/" "$MODULE_PATH/"
				else
					cp -a "${SUBMODULE}/.git/." "$MODULE_PATH/" || true
				fi
				printf "gitdir: ../.git/modules/libft\n" > "$SUBMODULE/.git"
				echo "Repaired by copying metadata into $MODULE_PATH"
				exit 0
			else
				echo "No embedded metadata found. Suggested manual steps:"
				echo "  git submodule deinit -f -- $SUBMODULE_REL"
				echo "  rm -rf $ROOT/.git/modules/libft"
				echo "  git submodule update --init --recursive $SUBMODULE_REL"
				exit 2
			fi
		fi
	fi
else
	echo "No $SUBMODULE/.git found. Likely not a valid submodule layout."
	echo "Try: (from repo root) git submodule update --init --recursive $SUBMODULE_REL"
	exit 1
fi

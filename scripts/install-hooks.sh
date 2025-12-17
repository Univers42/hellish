#!/bin/bash

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOOKS_SRC="$REPO_ROOT/scripts/hooks"
HOOKS_DST="$REPO_ROOT/.git/hooks"

declare -A HOOK_MAP=(
	["pre-commit"]="pre-commit"
	["commit-msg"]="commit-msg"
	["pre-push"]="pre-push"
	["post-checkout"]="post-checkout"
	["pre-merge-commit"]="pre-merge-commit"
)

echo "Resetting hooks in $HOOKS_DST ..."

for hook in "${!HOOK_MAP[@]}"; do
	dst="$HOOKS_DST/$hook"
	# Remove any existing file or symlink
	if [ -e "$dst" ] || [ -L "$dst" ]; then
		rm -f "$dst"
		echo "Removed old hook: $dst"
	fi
done

for hook in "${!HOOK_MAP[@]}"; do
	src="$HOOKS_SRC/${HOOK_MAP[$hook]}"
	dst="$HOOKS_DST/$hook"
	if [ -f "$src" ]; then
		ln -sf "$src" "$dst"
		chmod +x "$src"
		echo "Symlinked $dst -> $src"
	else
		echo "Source hook not found: $src"
	fi
done

# Symlink log_hook.sh for sourcing in hooks
if [ -f "$HOOKS_SRC/log_hook.sh" ]; then
	ln -sf "$HOOKS_SRC/log_hook.sh" "$HOOKS_DST/log_hook.sh"
	echo "Symlinked $HOOKS_DST/log_hook.sh -> $HOOKS_SRC/log_hook.sh"
else
	echo "Source log_hook.sh not found: $HOOKS_SRC/log_hook.sh"
fi

echo "All hooks and log_hook.sh reset and symlinked to .git/hooks/."

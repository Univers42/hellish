#!/usr/bin/env bash

set -e
REPO_DIR="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_DIR"

if [ ! -d "libft" ] || [ ! -f "libft/Makefile" ]; then
    echo "[hooks] init libft submodule..."
    git submodule update --init --recursive libft || true
fi

if [ -f "libft/Makefile" ]; then
    echo "[hooks] building libft..."
    (cd libft && make || true)
fi
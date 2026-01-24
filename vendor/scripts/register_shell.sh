#!/bin/sh

SHELL_PATH="/usr/bin/hellish"

echo "Registering shell..."

if ! grep -qx "$SHELL_PATH" /etc/shells; then
	echo "$SHELL_PATH" | sudo tee -a /etc/shells > /dev/null
else
	echo "Shell already registered"
fi

echo "Setting default shell for $USER"
chsh -s "$SHELL_PATH"
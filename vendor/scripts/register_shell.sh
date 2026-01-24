#!/bin/sh

SHELL_PATH="/usr/bin/hellish"

if ! grep -qx "$SHELL_PATH" /etc/shells; then
	echo "Registering shell in /etc/shells"
	echo "$SHELL_PATH" | sudo tee -a /etc/shells > /dev/null
else
	echo "Shell already registered"
fi

echo "Changing default shell for $USER"
chsh -s "$SHELL_PATH"

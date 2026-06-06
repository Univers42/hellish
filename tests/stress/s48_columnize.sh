#!/bin/sh
# Align a name/number table by padding the first column to the widest name.
# Two heredoc-fed passes (current shell), so the computed width survives.
maxw=0
while read -r name num; do
	[ ${#name} -gt $maxw ] && maxw=${#name}
done <<'DATA'
apple 3
banana 12
cherry 100
fig 7
DATA
while read -r name num; do
	pad=$((maxw - ${#name}))
	spaces=""; i=0
	while [ $i -lt $pad ]; do spaces="$spaces "; i=$((i + 1)); done
	printf '%s%s | %s\n' "$name" "$spaces" "$num"
done <<'DATA'
apple 3
banana 12
cherry 100
fig 7
DATA

#!/bin/sh
# Parse a simple key=value config (skipping blanks and # comments) into eval
# variables, then query them with a default for the missing key.
while IFS='=' read -r key val; do
	case "$key" in
	''|\#*) continue ;;
	esac
	eval "cfg_$key=\$val"
done <<'CFG'
host=localhost
port=8080
# a comment
user=admin
retries=5
CFG
for k in host port user retries missing; do
	v=$(eval "printf '%s' \"\${cfg_$k}\"")
	printf '%-8s = %s\n' "$k" "${v:-<unset>}"
done

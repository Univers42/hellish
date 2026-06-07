#!/usr/bin/env bash
# ============================================================================
# cd_zsh_compare.sh -- verify hellish's zsh-style two-argument `cd old new`
# extension against the shell it is modelled on: zsh. The bash suite cannot
# cover this form (bash rejects two operands as "too many arguments"), so this
# is its dedicated oracle. It is meant to run INSIDE the docker image built
# from docker/Dockerfile.zsh, where both hellish and zsh exist; the host has
# no zsh. See `make cd-zsh-test`.
#
# Each case is run through hellish and zsh from the same working directory and
# their stdout + exit status are compared (the meaningful contract: the printed
# destination and success/failure). Stderr wording differs by shell name and is
# shown for context but not gated on.
# ============================================================================
set -u

HELLISH="${HELLISH:-/hellish/build/bin/hellish}"
ZSH="$(command -v zsh || true)"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1

if [ ! -x "$HELLISH" ]; then echo "error: hellish not found at $HELLISH" >&2; exit 2; fi
if [ -z "$ZSH" ]; then echo "error: zsh not installed in this image" >&2; exit 2; fi

# A sandbox with a repeated component (/s/a/a, plus the /s/b/a that replacing
# the FIRST "a" lands on) so first-occurrence replacement is observable, plus
# sibling targets for the basic substitution.
SB=/tmp/cdzsh
rm -rf "$SB"; mkdir -p "$SB/aaa/x" "$SB/bbb/x" "$SB/s/a/a" "$SB/s/b/a"

strip_h(){ sed 's/\x1b\[[0-9;]*[A-Za-z]//g' | grep -vE '^[[:space:]]*❯' | grep -v '^exit$'; }

# Each entry: "<start-dir>|||<command>"
cases=(
  "$SB/aaa|||cd aaa bbb && pwd"
  "$SB/aaa|||cd aaa bbb"
  "$SB/aaa|||cd aaa bbb; echo rc=\$?"
  "$SB/aaa|||cd zzz www; echo rc=\$?"
  "$SB/s/a/a|||cd a b 2>/dev/null; echo rc=\$?"
  "$SB/s/a/a|||cd a b 2>/dev/null && pwd"
  "$SB/s/a/a|||cd a b && pwd"
  "$SB/aaa|||cd aaa bbb ccc; echo rc=\$?"
  "$SB/aaa/x|||cd aaa bbb && pwd"
)

pass=0; fail=0
printf '\n\033[1m== hellish vs zsh: two-argument cd ==\033[0m\n'
for entry in "${cases[@]}"; do
  start="${entry%%|||*}"
  cmd="${entry##*|||}"
  ho=$(cd "$start"; "$HELLISH" -c "$cmd" 2>/tmp/zh_e); hx=$?
  ho=$(printf '%s' "$ho" | strip_h)
  zo=$(cd "$start"; "$ZSH" -f -c "$cmd" 2>/tmp/zz_e); zx=$?
  if [ "$ho" = "$zo" ] && [ "$hx" = "$zx" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %s\n' "$cmd"
  else
    fail=$((fail+1))
    printf '  \033[31mFAIL\033[0m %s\n' "$cmd"
    printf '       hellish(rc=%s) stdout=[%s] err=[%s]\n' "$hx" "$ho" "$(head -1 /tmp/zh_e)"
    printf '       zsh    (rc=%s) stdout=[%s] err=[%s]\n' "$zx" "$zo" "$(head -1 /tmp/zz_e)"
  fi
done

printf '\n\033[1m== %d match, %d differ (zsh %s) ==\033[0m\n' \
  "$pass" "$fail" "$("$ZSH" --version 2>/dev/null | head -1)"
[ "$fail" -eq 0 ]

#!/usr/bin/env bash
# ============================================================================
# cd_zsh_compare.sh -- verify hellish's zsh-only `cd` behaviour (the
# two-argument `cd old new` extension, and -q) against the shell it is
# modelled on: zsh. The bash suite cannot
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
  # -q: zsh moves quietly. `emulate zsh` is what arms hellish's dialect and
  # is a no-op in zsh itself, so one string still describes both shells.
  # oh-my-zsh's extract plugin runs `builtin cd -q` four times; before this
  # was accepted, every archive it opened printed "cd: -q: invalid option"
  # and the final restore of the caller's directory failed.
  "$SB/aaa|||emulate zsh; cd -q ../bbb && pwd"
  "$SB/aaa|||emulate zsh; cd -q ../bbb; echo rc=$?"
  "$SB/aaa|||emulate zsh; cd -qP ../bbb && pwd"
  "$SB/aaa|||emulate zsh; cd -Pq ../bbb && pwd"
  "$SB/aaa|||emulate zsh; cd -q -- ../bbb && pwd"
  "$SB/aaa|||emulate zsh; cd -q ../nope; echo rc=$?"
  "$SB/aaa|||emulate zsh; cd -qz ../bbb; echo rc=$?"
  # -q suppresses the chpwd hook; a plain cd still fires it.
  "$SB/aaa|||emulate zsh; chpwd() { echo HOOK; }; cd ../bbb; cd -q ../aaa"
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

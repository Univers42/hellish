#!/usr/bin/env bash
# ============================================================================
# cd_posix_compare.sh -- verify hellish's POSIX mode (`--posix`) against the
# shell it must match there: bash --posix. In POSIX mode the zsh-style
# two-argument `cd old new` extension is disabled, so two (or more) operands
# become the bash "too many arguments" error (exit 2 since bash 5.3) --
# exactly like bash.
#
# This complements `make cd-zsh-test` (which checks the *extension* in the
# default, non-POSIX mode against real zsh): here we check the *absence* of the
# extension under --posix, plus a guard that the extension is still present in
# normal mode. It runs on the host -- it needs only bash + hellish, no docker.
#
# Each case is run through `hellish --posix -c` and `bash --posix -c` from the
# same directory; stdout + exit status are compared. Stderr wording differs by
# shell name and is not gated on.
# ============================================================================
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
# The bash in PATH is the SPECIFICATION for these cases, not an environment
# detail, and it changes POSIX-visible behaviour between minor releases --
# cd's status on too many operands is exit 1 up to bash 5.2 and exit 2 from
# 5.3. Grading against whatever bash the host ships reports that drift as a
# hellish bug. Prefer the pinned oracle `make oracle` builds, and say plainly
# when we are grading against something else.
ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
if [ -x "$ORACLE_HOME/bin/bash" ]; then
	PATH="$ORACLE_HOME/bin:$PATH"
	export PATH
fi
case "$(bash --version 2>/dev/null | head -1)" in
	*"version 5.3"*) ;;
	*) printf '\033[33m!  grading against bash %s, not the pinned 5.3.9 -- run `make oracle`\033[0m\n' \
		"$(bash --version 2>/dev/null | head -1 | sed 's/.*version \([0-9.]*\).*/\1/')" >&2 ;;
esac

HELLISH="${HELLISH:-$HERE/../build/bin/hellish}"
# Absolutise: the cases cd into sandbox dirs before invoking hellish, so a
# relative path (e.g. from `make`) would stop resolving.
HELLISH="$(cd "$(dirname "$HELLISH")" 2>/dev/null && pwd)/$(basename "$HELLISH")"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1

if [ ! -x "$HELLISH" ]; then echo "error: hellish not found at $HELLISH" >&2; exit 2; fi

strip_h(){ sed 's/\x1b\[[0-9;]*[A-Za-z]//g' | grep -vE '^[[:space:]]*❯' | grep -v '^exit$'; }

SB=/tmp/cdposix
rm -rf "$SB"; mkdir -p "$SB/aaa/x" "$SB/bbb/x"

# Each entry: "<start-dir>|||<command>". These are cases where the default mode
# would apply the two-arg extension; under --posix both shells must agree.
cases=(
  "$SB/aaa|||cd aaa bbb; echo rc=\$?"
  "$SB/aaa|||cd aaa bbb && pwd; echo rc=\$?"
  "$SB/aaa|||cd aaa bbb ccc; echo rc=\$?"
  "$SB/aaa|||cd /tmp && pwd"
  "$SB/aaa|||cd -- /tmp && pwd"
)

pass=0; fail=0
printf '\n\033[1m== hellish --posix vs bash --posix: cd ==\033[0m\n'
for entry in "${cases[@]}"; do
  start="${entry%%|||*}"
  cmd="${entry##*|||}"
  ho=$(cd "$start"; "$HELLISH" --posix -c "$cmd" 2>/dev/null); hx=$?
  ho=$(printf '%s' "$ho" | strip_h)
  bo=$(cd "$start"; bash --posix -c "$cmd" 2>/dev/null); bx=$?
  if [ "$ho" = "$bo" ] && [ "$hx" = "$bx" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %s\n' "$cmd"
  else
    fail=$((fail+1))
    printf '  \033[31mFAIL\033[0m %s\n' "$cmd"
    printf '       hellish(rc=%s) stdout=[%s]\n' "$hx" "$ho"
    printf '       bash   (rc=%s) stdout=[%s]\n' "$bx" "$bo"
  fi
done

# Guard: in normal (non-POSIX) mode the extension must STILL apply.
printf '\n\033[1m== normal mode: two-arg extension intact ==\033[0m\n'
got=$(cd "$SB/aaa"; "$HELLISH" -c 'cd aaa bbb && pwd' 2>/dev/null | strip_h)
if [ "$got" = "$SB/bbb" ]; then
  pass=$((pass+1)); printf '  \033[32mOK\033[0m   cd aaa bbb -> %s\n' "$got"
else
  fail=$((fail+1)); printf '  \033[31mFAIL\033[0m cd aaa bbb -> [%s] (want %s)\n' "$got" "$SB/bbb"
fi

# Guard: `set -o posix` at runtime gates the extension the same way --posix
# does. The expected status comes from live bash --posix (1 before bash 5.3,
# 2 from 5.3 on) so the guard tracks the same oracle as the golden suite.
got=$(cd "$SB/aaa"; "$HELLISH" -c 'set -o posix; cd aaa bbb 2>/dev/null; echo rc=$?' | strip_h)
want=$(cd "$SB/aaa"; bash --posix -c 'cd aaa bbb 2>/dev/null; echo rc=$?')
if [ "$got" = "$want" ]; then
  pass=$((pass+1)); printf '  \033[32mOK\033[0m   set -o posix; cd aaa bbb -> %s\n' "$got"
else
  fail=$((fail+1)); printf '  \033[31mFAIL\033[0m set -o posix; cd aaa bbb -> [%s] (want %s)\n' "$got" "$want"
fi

printf '\n\033[1m== %d pass, %d fail (bash %s) ==\033[0m\n' \
  "$pass" "$fail" "$(bash --version 2>/dev/null | head -1)"
[ "$fail" -eq 0 ]

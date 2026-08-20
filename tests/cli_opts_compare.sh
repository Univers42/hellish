#!/usr/bin/env bash
# ============================================================================
# cli_opts_compare.sh -- verify hellish's command-line option parsing against
# bash --posix.  These cases can't live in the golden category files: the
# harness wraps every line in `<shell> -c`, so it can't exercise how the
# shell parses its OWN argv (-e, -o name, +c, flags between -c and the
# command string, `--`/`-`, invalid-option status, $-).  Here each row is
# run by invoking the shell binary directly and diffing stdout + exit status.
#
# Runs on the host: needs only bash + hellish, no docker.  Stderr wording
# differs by shell and is not gated on.
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
HELLISH="$(cd "$(dirname "$HELLISH")" 2>/dev/null && pwd)/$(basename "$HELLISH")"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1

if [ ! -x "$HELLISH" ]; then echo "error: hellish not found at $HELLISH" >&2; exit 2; fi

strip_h(){ sed 's/\x1b\[[0-9;]*[A-Za-z]//g' | grep -vE '^[[:space:]]*❯' | grep -v '^exit$'; }

# Each row is a full argv (after the shell name), passed verbatim to both
# shells.  We compare stdout + status; $- rows print flags so their stdout
# carries the interesting part.  bash is invoked with --norc --posix so it
# matches hellish's --posix posture.
rows=(
  "-e -c |false; echo no"
  "-o errexit -c |false; echo no"
  "-eu -c |echo hi"
  "-c -x |echo hi"
  "-c -- |echo two"
  "-c - |echo one"
  "-c -e |false; echo no"
  "-c |set -u; : \${missing}; echo after"
  "-l -c |exit 0"
)

pass=0; fail=0
printf '\n\033[1m== hellish vs bash --posix: CLI option parsing ==\033[0m\n'
run_row() {
  local pre="${1%%|*}" cmd="${1#*|}"
  # shellcheck disable=SC2086
  ho=$("$HELLISH" --posix $pre "$cmd" 2>/dev/null); hx=$?
  ho=$(printf '%s' "$ho" | strip_h)
  # shellcheck disable=SC2086
  bo=$(bash --norc --posix $pre "$cmd" 2>/dev/null); bx=$?
  if [ "$ho" = "$bo" ] && [ "$hx" = "$bx" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %s%s\n' "$pre" "$cmd"
  else
    fail=$((fail+1))
    printf '  \033[31mFAIL\033[0m %s%s\n' "$pre" "$cmd"
    printf '       hellish(rc=%s) [%s]\n' "$hx" "$ho"
    printf '       bash   (rc=%s) [%s]\n' "$bx" "$bo"
  fi
}
for r in "${rows[@]}"; do run_row "$r"; done

# Status-only rows: invalid options must abort with bash's usage status 2.
printf '\n\033[1m== invalid-option status ==\033[0m\n'
inval=("-z" "-c -z" "-c ---" "+q")
for args in "${inval[@]}"; do
  # shellcheck disable=SC2086
  "$HELLISH" --posix $args >/dev/null 2>&1 </dev/null; hx=$?
  # shellcheck disable=SC2086
  bash --norc --posix $args >/dev/null 2>&1 </dev/null; bx=$?
  if [ "$hx" = "$bx" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %-8s rc=%s\n' "$args" "$hx"
  else
    fail=$((fail+1)); printf '  \033[31mFAIL\033[0m %-8s hellish rc=%s bash rc=%s\n' "$args" "$hx" "$bx"
  fi
done

# Script/stdin nounset exits 1 (bash), where -c mode exits 127; both shells
# must agree per mode.  A real script file exercises the INP_FILE path.
printf '\n\033[1m== nounset exit status by mode ==\033[0m\n'
SCR="$(mktemp)"; printf 'set -u\n: ${missing}\necho after\n' > "$SCR"
hx=$("$HELLISH" --posix "$SCR" >/dev/null 2>&1; echo $?)
bx=$(bash --norc --posix "$SCR" >/dev/null 2>&1; echo $?)
his=$(printf 'set -u\n: ${missing}\necho after\n' | "$HELLISH" --posix >/dev/null 2>&1; echo $?)
bis=$(printf 'set -u\n: ${missing}\necho after\n' | bash --norc --posix >/dev/null 2>&1; echo $?)
rm -f "$SCR"
for pair in "script $hx $bx" "stdin $his $bis"; do
  set -- $pair
  if [ "$2" = "$3" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %-7s nounset rc=%s\n' "$1" "$2"
  else
    fail=$((fail+1)); printf '  \033[31mFAIL\033[0m %-7s hellish rc=%s bash rc=%s\n' "$1" "$2" "$3"
  fi
done

# $- must carry 'i' when -i is given, and must NOT when it isn't.
printf '\n\033[1m== $- interactive marker ==\033[0m\n'
gi=$("$HELLISH" --posix -i -c 'echo $-' 2>/dev/null | grep -c i)
gn=$("$HELLISH" --posix -c 'echo $-' 2>/dev/null | grep -c i)
if [ "$gi" = 1 ] && [ "$gn" = 0 ]; then
  pass=$((pass+1)); printf '  \033[32mOK\033[0m   -i adds i (%s), plain omits it (%s)\n' "$gi" "$gn"
else
  fail=$((fail+1)); printf '  \033[31mFAIL\033[0m -i=%s plain=%s (want 1/0)\n' "$gi" "$gn"
fi

# --version: exit 0, one greppable first line carrying the version, and it
# must NOT read stdin or run a startup file (package managers and CI probe
# with it). The version it prints has to be the one baked into version.h,
# otherwise a release can ship a binary that misreports itself.
printf '\n\033[1m== --version ==\033[0m\n'
want=$(grep -oE '"[0-9]+\.[0-9]+\.[0-9]+"' "$HERE/../incs/version.h" | head -1 | tr -d '"')
vout=$(printf 'echo SHOULD_NOT_RUN\n' | "$HELLISH" --version 2>&1); vrc=$?
vfirst=$(printf '%s\n' "$vout" | head -1)
for c in "exit status 0:$vrc:0" \
         "reports version.h version:$(printf '%s' "$vfirst" | grep -c "$want"):1" \
         "first line names the shell:$(printf '%s' "$vfirst" | grep -c '^hellish'):1" \
         "does not execute stdin:$(printf '%s' "$vout" | grep -c SHOULD_NOT_RUN):0"; do
  name=${c%%:*}; rest=${c#*:}; got=${rest%%:*}; exp=${rest#*:}
  if [ "$got" = "$exp" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %s\n' "$name"
  else
    fail=$((fail+1)); printf '  \033[31mFAIL\033[0m %s (got %s want %s)\n' "$name" "$got" "$exp"
  fi
done

printf '\n\033[1m== %d pass, %d fail (bash %s) ==\033[0m\n' \
  "$pass" "$fail" "$(bash --version 2>/dev/null | head -1)"
[ "$fail" -eq 0 ]

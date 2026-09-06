#!/usr/bin/env bash
# ============================================================================
# path_lookup_compare.sh -- what `command -v`, `type` and `hash` name when the
# only file of that name on PATH is not executable, against bash.
#
# bash's PATH search prefers an executable and, failing that, still names the
# first plain file it found (findcmd.c, FS_EXEC_PREFERRED): `command -v` prints
# it and exits 0, `type` says "is", `type -t` says "file"; running it is
# "Permission denied", 126; `hash` alone refuses it.  bash --posix drops the
# fallback (type.def checks FS_EXECABLE under posixly_correct).  A 42
# workstation's /usr/bin/sudo is 4750 root:sudo, so this is exactly what
# born2root's vm_path.sh sees when it asks `command -v sudo` -- and hellish
# said "not found" there, printed the no-sudo recipe, and failed the corpus's
# unit test.
#
# The golden suite cannot hold this: it grades against bash --posix, where the
# two behaviours coincide on "not found".  So each row runs under plain bash
# and plain hellish, then under both --posix; stdout + status are compared.
# Runs on the host: needs only bash + hellish, no root, no docker.
# ============================================================================
HERE="$(cd "$(dirname "$0")" && pwd)"
ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
HELLISH="${HELLISH:-$HERE/../build/bin/hellish}"
HELLISH="$(cd "$(dirname "$HELLISH")" 2>/dev/null && pwd)/$(basename "$HELLISH")"
[ -x "$HELLISH" ] || { echo "error: $HELLISH not found (make all)" >&2; exit 2; }
if [ -x "$ORACLE_HOME/bin/bash" ]; then
  PATH="$ORACLE_HOME/bin:$PATH"; export PATH
fi
BASH_BIN="$(command -v bash)"
printf '  oracle: %s (%s)\n' "$BASH_BIN" "$(bash --version | head -1 | sed 's/.*version \([0-9.]*\).*/\1/')"

# A PATH of our own: `plain` exists and is not executable, `runs` is, `both`
# is non-executable first and executable further along, `adir` is a directory.
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
mkdir -p "$T/a" "$T/b" "$T/a/adir"
: > "$T/a/plain"; chmod 644 "$T/a/plain"
printf '#!/bin/sh\necho ran\n' > "$T/b/runs"; chmod 755 "$T/b/runs"
: > "$T/a/both"; chmod 644 "$T/a/both"
printf '#!/bin/sh\necho both-runs\n' > "$T/b/both"; chmod 755 "$T/b/both"
: > "$T/a/nope"; chmod 000 "$T/a/nope"
P="$T/a:$T/b"

rows=(
  'command -v plain; echo "st=$?"'
  'type plain; echo "st=$?"'
  'type -t plain; echo "st=$?"'
  'type -p plain; echo "st=$?"'
  'type -P plain; echo "st=$?"'
  'command -V plain; echo "st=$?"'
  'hash plain 2>/dev/null; echo "st=$?"'
  'plain 2>/dev/null; echo "st=$?"'
  'command -v nope; echo "st=$?"'
  'command -v both; echo "st=$?"'
  'type both'
  'both'
  'command -v runs; echo "st=$?"'
  'command -v adir; echo "st=$?"'
  'type -t adir; echo "st=$?"'
  'command -v missing; echo "st=$?"'
  'command -v plain runs; echo "st=$?"'
  'command -v missing runs; echo "st=$?"'
  'command -v runs missing; echo "st=$?"'
  'command -V runs missing 2>/dev/null; echo "st=$?"'
  'command -V plain missing 2>/dev/null; echo "st=$?"'
  'if command -v plain >/dev/null 2>&1; then echo have; else echo none; fi'
  'if ! command -v plain >/dev/null 2>&1; then echo none; else echo have; fi'
)

pass=0; fail=0
run_row() { # run_row <label> <hellish args> <bash args> <snippet>
  local label="$1" hargs="$2" bargs="$3" cmd="$4" ho hx bo bx
  # shellcheck disable=SC2086
  ho=$(PATH="$P" "$HELLISH" $hargs -c "$cmd" 2>/dev/null | sed "s#$T#T#g"); hx=${PIPESTATUS[0]}
  # shellcheck disable=SC2086
  bo=$(PATH="$P" "$BASH_BIN" --norc $bargs -c "$cmd" 2>/dev/null | sed "s#$T#T#g"); bx=${PIPESTATUS[0]}
  if [ "$ho" = "$bo" ] && [ "$hx" = "$bx" ]; then
    pass=$((pass+1)); printf '  \033[32mOK\033[0m   %-8s %s\n' "$label" "$cmd"
  else
    fail=$((fail+1))
    printf '  \033[31mFAIL\033[0m %-8s %s\n' "$label" "$cmd"
    printf '       hellish(rc=%s) [%s]\n' "$hx" "$(printf '%s' "$ho" | tr '\n' '|')"
    printf '       bash   (rc=%s) [%s]\n' "$bx" "$(printf '%s' "$bo" | tr '\n' '|')"
  fi
}
printf '\n\033[1m== hellish vs bash: a non-executable file on PATH ==\033[0m\n'
for r in "${rows[@]}"; do run_row "default" "" "" "$r"; done
printf '\n\033[1m== hellish --posix vs bash --posix ==\033[0m\n'
for r in "${rows[@]}"; do run_row "--posix" "--posix" "--posix" "$r"; done

printf '\n%d ok, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]

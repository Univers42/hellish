#!/bin/sh
# Compare hellish vs bash --posix (and plain bash) on each hard script.
# Usage: ./run.sh [script.sh ...]   (default: all hard/*.sh)
H="${HELLISH_BIN:-$(cd "$(dirname "$0")/../.." && pwd)/build/bin/hellish}"
if [ ! -x "$H" ] && [ -x "$H.exe" ]; then H="$H.exe"; fi
DIR="$(cd "$(dirname "$0")" && pwd)"
scripts="$*"; [ -z "$scripts" ] && scripts="$(ls "$DIR"/[0-9]*.sh 2>/dev/null)"
pass=0; fail=0
for s in $scripts; do
  name=$(basename "$s")
  ho=$("$H" "$s" 2>&1); hc=$?
  bo=$(bash --posix "$s" 2>&1); bc=$?
  if [ "$ho" = "$bo" ] && [ "$hc" = "$bc" ]; then
    # timing best-of-3
    hb=999999; bb=999999; i=0
    while [ $i -lt 3 ]; do
      t0=$(date +%s%N); "$H" "$s" >/dev/null 2>&1; t1=$(date +%s%N); d=$(( (t1-t0)/1000000 )); [ $d -lt $hb ] && hb=$d
      t0=$(date +%s%N); bash --posix "$s" >/dev/null 2>&1; t1=$(date +%s%N); d=$(( (t1-t0)/1000000 )); [ $d -lt $bb ] && bb=$d
      i=$((i+1))
    done
    spd="="; [ "$hb" -lt "$bb" ] && spd="FASTER"; [ "$hb" -gt "$bb" ] && spd="slower"
    printf "PASS  %-26s rc=%s  h=%4dms b=%4dms %s\n" "$name" "$hc" "$hb" "$bb" "$spd"
    pass=$((pass+1))
  else
    printf "FAIL  %-26s hc=%s bc=%s\n" "$name" "$hc" "$bc"
    diff <(printf '%s' "$ho") <(printf '%s' "$bo") | head -25 | sed 's/^/      /'
    fail=$((fail+1))
  fi
done
printf -- "---- %d PASS / %d FAIL ----\n" "$pass" "$fail"

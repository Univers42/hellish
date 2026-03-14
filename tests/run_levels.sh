#!/bin/bash
HELLISH="/home/dylan/sh42/build/bin/hellish"
PASS=0
FAIL=0
LEAK=0

for i in $(seq 1 20); do
  f=$(printf "/home/dylan/sh42/tests/level%02d.sh" $i)
  out=$($HELLISH "$f" 2>/tmp/lvl_stderr.txt)
  ex=$?
  leaks=$(grep -c "LeakSanitizer" /tmp/lvl_stderr.txt 2>/dev/null)
  if [ "$leaks" -gt 0 ]; then
    echo "LEVEL $i: LEAK ($leaks leak reports)"
    LEAK=$((LEAK+1))
  else
    echo "LEVEL $i: OK"
    PASS=$((PASS+1))
  fi
done

echo "=== RESULTS: $PASS pass, $FAIL fail, $LEAK leak ==="

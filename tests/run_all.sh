#!/bin/bash
export ASAN_OPTIONS=detect_leaks=0
SHELL_BIN="/home/dylan/sh42/build/bin/hellish"
TEST_DIR="/home/dylan/sh42/tests"
LOG="/home/dylan/sh42/tests/results.log"

echo "============================================" > "$LOG"
echo "  HELLISH SHELL TEST RESULTS" >> "$LOG"
echo "  $(date)" >> "$LOG"
echo "============================================" >> "$LOG"
echo "" >> "$LOG"

for level in $(seq -w 1 20); do
  SCRIPT="$TEST_DIR/level${level}.sh"
  if [ -f "$SCRIPT" ]; then
    echo "---[ level${level}.sh ]---" >> "$LOG"
    cat "$SCRIPT" | "$SHELL_BIN" 2>/dev/null >> "$LOG" 2>&1
    echo "" >> "$LOG"
    echo "--- exit: $? ---" >> "$LOG"
    echo "" >> "$LOG"
  fi
done

echo "============================================" >> "$LOG"
echo "  ALL TESTS COMPLETE" >> "$LOG"
echo "============================================" >> "$LOG"

cat "$LOG"

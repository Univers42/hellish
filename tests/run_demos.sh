#!/bin/bash
HELLISH=/home/dylan/sh42/build/bin/hellish
DIR=/home/dylan/sh42/tests
PASS=0
FAIL=0

for f in demo_functions.sh demo_multiline.sh demo_shell_txt.sh; do
    OUT=$($HELLISH "$DIR/$f" 2>&1)
    if echo "$OUT" | grep -q "LeakSanitizer\|AddressSanitizer"; then
        echo "$f: LEAK"
        FAIL=$((FAIL + 1))
    else
        echo "$f: OK"
        PASS=$((PASS + 1))
    fi
done

echo "=== RESULTS: $PASS pass, $FAIL fail ==="

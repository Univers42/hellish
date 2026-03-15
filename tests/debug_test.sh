#!/bin/bash
H=/home/dylan/sh42/build/bin/hellish
PASS=0
FAIL=0

for f in /home/dylan/sh42/tests/level*.sh /home/dylan/sh42/tests/demo_*.sh; do
    timeout 10 $H "$f" > /tmp/hout 2>/tmp/herr
    E=$?
    if grep -q "SUMMARY" /tmp/herr 2>/dev/null; then
        echo "LEAK $(basename $f)"
        FAIL=$((FAIL + 1))
    elif [ $E -eq 141 ] || [ $E -gt 128 ]; then
        echo "CRASH $(basename $f) (exit=$E)"
        FAIL=$((FAIL + 1))
    else
        echo "OK   $(basename $f)"
        PASS=$((PASS + 1))
    fi
done

echo ""
echo "=== $PASS OK, $FAIL FAIL ==="

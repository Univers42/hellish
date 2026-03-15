#!/bin/bash
HELLISH=/home/dylan/sh42/build/bin/hellish
DIR=/home/dylan/sh42/tests
PASS=0
FAIL=0
FAILS=""

for f in level01.sh level02.sh level03.sh level04.sh level05.sh \
         level06.sh level07.sh level08.sh level09.sh level10.sh \
         level11.sh level12.sh level13.sh level14.sh level15.sh \
         level16.sh level17.sh level18.sh level19.sh level20.sh \
         demo_functions.sh demo_multiline.sh demo_shell_txt.sh; do

    BASH_OUT=$(bash "$DIR/$f" 2>/dev/null)
    HELL_OUT=$($HELLISH "$DIR/$f" 2>/dev/null)

    if [ "$BASH_OUT" = "$HELL_OUT" ]; then
        echo "OK   $f"
        PASS=$((PASS + 1))
    else
        echo "DIFF $f"
        diff <(echo "$BASH_OUT") <(echo "$HELL_OUT") | head -30
        echo "---"
        FAIL=$((FAIL + 1))
        FAILS="$FAILS $f"
    fi
done

echo ""
echo "=== RESULTS: $PASS pass, $FAIL diff ==="
if [ -n "$FAILS" ]; then
    echo "FAILING:$FAILS"
fi

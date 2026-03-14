echo "============================================"
echo "  HELLISH SHELL - MULTI-LINE DEMO"
echo "============================================"
echo ""

echo "--- 1. Multi-line IF/ELIF/ELSE ---"
X=42
if [ $X -gt 100 ]
then
    echo "X is greater than 100"
elif [ $X -gt 30 ]
then
    echo "X is between 31 and 100"
else
    echo "X is 30 or less"
fi

echo ""
echo "--- 2. Multi-line FOR loop ---"
for fruit in apple banana cherry mango
do
    echo "I like $fruit"
done

echo ""
echo "--- 3. Multi-line WHILE loop ---"
COUNT=0
while [ $COUNT -lt 5 ]
do
    echo "  counting: $COUNT"
    COUNT=$(($COUNT + 1))
done
echo "  final count: $COUNT"

echo ""
echo "--- 4. Multi-line UNTIL loop ---"
N=3
until [ $N -eq 0 ]
do
    echo "  countdown: $N"
    N=$(($N - 1))
done
echo "  LIFTOFF!"

echo ""
echo "--- 5. Nested IF inside FOR ---"
for num in 1 2 3 4 5 6
do
    if [ $(($num % 2)) -eq 0 ]
    then
        echo "  $num is even"
    else
        echo "  $num is odd"
    fi
done

echo ""
echo "--- 6. Nested FOR inside WHILE ---"
I=0
while [ $I -lt 3 ]
do
    echo "  outer=$I:"
    for letter in x y z
    do
        echo "    inner=$letter"
    done
    I=$(($I + 1))
done

echo ""
echo "--- 7. Multi-line with pipes ---"
for word in hello world foo bar baz
do
    echo $word
done
echo "charlie" | cat
echo "alpha" | tr a-z A-Z

echo ""
echo "--- 8. Multi-line with redirections ---"
echo "redirect test" > /tmp/hellish_demo_out.txt
echo "append test" >> /tmp/hellish_demo_out.txt
cat /tmp/hellish_demo_out.txt
rm -f /tmp/hellish_demo_out.txt
echo "  heredoc test:"
cat << EOF
hello from heredoc
EOF

echo ""
echo "--- 9. Multi-line with variables and arithmetic ---"
SUM=0
for val in 10 20 30 40
do
    SUM=$(($SUM + $val))
    echo "  added $val, sum=$SUM"
done
echo "  total: $SUM"

echo ""
echo "--- 10. Deep nesting (3 levels) ---"
for a in 1 2
do
    for b in X Y
    do
        if [ $a -eq 1 ]
        then
            echo "  $a-$b (first)"
        else
            echo "  $a-$b (second)"
        fi
    done
done

echo ""
echo "--- 11. Multi-line subshell ---"
X=outer
(
    X=inner
    echo "  subshell X=$X"
)
echo "  parent X=$X"

echo ""
echo "--- 12. Multi-line with && and || ---"
true && if [ 1 -eq 1 ]
then
    echo "  AND chain into if: works"
fi

false || for word in fallback
do
    echo "  OR chain into for: $word"
done

echo ""
echo "--- 13. Parameter expansion formats ---"
unset MISSING
echo "  default: ${MISSING:-default_value}"
echo "  alt (set): ${SUM:+has_value}"
echo "  length: ${#SUM}"

echo ""
echo "--- 14. Globbing and wildcards ---"
echo "  /etc/host*:"
ls /etc/host*
echo "  /tmp/hellish_glob_test:"
echo test1 > /tmp/hgt_a.txt
echo test2 > /tmp/hgt_b.txt
ls /tmp/hgt_?.txt
rm -f /tmp/hgt_a.txt /tmp/hgt_b.txt

echo ""
echo "--- 15. Arithmetic expansion ---"
echo "  2+3 = $((2+3))"
echo "  10*7 = $((10*7))"
echo "  100/3 = $((100/3))"
echo "  2**8 = $((2**8))"
echo "  17%5 = $((17%5))"

echo ""
echo "============================================"
echo "  ALL MULTI-LINE DEMOS COMPLETE"
echo "============================================"

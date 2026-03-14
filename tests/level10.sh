echo === LEVEL 10: Advanced Combinations ===
echo "cmd sub: $(echo hello from subshell)"
echo "arith: $((2 + 3 * 4))"
echo "nested arith: $(( (10 - 2) / 4 ))"
for i in 1 2 3; do echo "item $i" > /tmp/test_loop_$i; done
for i in 1 2 3; do cat /tmp/test_loop_$i; done
rm /tmp/test_loop_1 /tmp/test_loop_2 /tmp/test_loop_3
RESULT=$(for x in hello world; do echo $x; done)
echo "captured: $RESULT"
echo "length: ${#RESULT}"
NAME=world
echo "default: ${UNSET_VAR:-fallback_value}"
echo "alt: ${NAME:+name is set}"
x=0
while [ $x -lt 3 ]; do if [ $x -eq 1 ]; then echo "skip $x" | tr a-z A-Z; else echo "normal $x"; fi; x=$((x+1)); done
(echo "in subshell"; MY_SUB=inner; echo $MY_SUB)
echo "after subshell: [$MY_SUB]"
echo "pipe + compound:" | cat && echo "and chain works"
for f in a b c; do echo $f; done | sort -r
true && if true; then echo "AND + IF: ok"; fi
echo "all tests complete"

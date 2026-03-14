echo === LEVEL 05: Operators and Exit Codes ===
true ; echo "after true: $?"
false ; echo "after false: $?"
true && echo "AND: true branch"
false && echo "AND: should not appear"
false || echo "OR: false branch"
true || echo "OR: should not appear"
true && echo "chain1" && echo "chain2"
false || echo "fallback1" || echo "should not appear"
true && false || echo "mixed: correct"
echo one ; echo two ; echo three

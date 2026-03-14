echo === LEVEL 09: Nested Compound Commands ===
for x in a b; do for y in 1 2; do echo "$x$y"; done; done
for i in 1 2 3; do if [ $i -eq 2 ]; then echo "found two"; else echo "not two: $i"; fi; done
x=0
while [ $x -lt 2 ]; do for letter in A B; do echo "loop $x: $letter"; done; x=$((x + 1)); done
if true; then for val in yes no; do echo "inside if: $val"; done; fi
for n in 1 2; do if [ $n -eq 1 ]; then echo "first" | cat; else echo "second" | tr a-z A-Z; fi; done

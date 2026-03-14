echo === LEVEL 14: Nested Multi-line Compounds ===
for x in a b; do
  for y in 1 2; do
    echo "$x$y"
  done
done
for i in 1 2 3; do
  if [ $i -eq 2 ]; then
    echo "found two"
  else
    echo "not two: $i"
  fi
done
if true; then
  for v in hello world; do
    echo "inner: $v"
  done
fi
x=0
while [ $x -lt 2 ]; do
  for ch in A B; do
    echo "w$x: $ch"
  done
  x=$((x + 1))
done

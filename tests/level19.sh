echo === LEVEL 19: Deep Nesting Multi-line ===
for a in 1 2; do
  for b in x y; do
    for c in p q; do
      echo "$a-$b-$c"
    done
  done
done
i=0
while [ $i -lt 2 ]; do
  j=0
  while [ $j -lt 2 ]; do
    echo "($i,$j)"
    j=$((j+1))
  done
  i=$((i+1))
done
if true; then
  if true; then
    if true; then
      echo "triple nested if"
    fi
  fi
fi
for x in 1 2; do
  if [ $x -eq 1 ]; then
    for y in a b; do
      echo "x=$x y=$y"
    done
  else
    i=0
    while [ $i -lt 2 ]; do
      echo "x=$x i=$i"
      i=$((i+1))
    done
  fi
done

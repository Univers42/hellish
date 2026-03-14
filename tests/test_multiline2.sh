echo "=== NESTED MULTI-LINE ==="
for x in a b; do
  for y in 1 2; do
    echo "$x$y"
  done
done
echo "=== DEEPLY NESTED ==="
if true; then
  for i in 1 2 3; do
    if [ $i -eq 2 ]; then
      echo "hit: $i"
    else
      echo "miss: $i"
    fi
  done
fi
echo "=== WHILE + IF ==="
n=0
while [ $n -lt 3 ]; do
  if [ $n -eq 1 ]; then
    echo "special: $n"
  else
    echo "normal: $n"
  fi
  n=$((n+1))
done
echo "=== UNTIL MULTI-LINE ==="
k=3
until [ $k -eq 0 ]; do
  echo "until: $k"
  k=$((k-1))
done
echo "=== ALL DONE ==="

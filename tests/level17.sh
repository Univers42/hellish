echo === LEVEL 17: Multi-line + Variables and Arithmetic ===
total=0
for i in 1 2 3 4 5; do
  total=$((total + i))
  echo "running total: $total"
done
echo "grand total: $total"
base=2
exp=1
while [ $exp -le 8 ]; do
  echo "$base^? approx=$exp"
  exp=$((exp * 2))
done
for op in plus minus; do
  if [ "$op" = "plus" ]; then
    echo "10 + 5 = $((10 + 5))"
  else
    echo "10 - 5 = $((10 - 5))"
  fi
done
MSG="hello world"
echo "length of MSG: ${#MSG}"
echo "default: ${UNSET:-fallback}"
NAME=test
echo "alt: ${NAME:+is set}"

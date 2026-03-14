echo === LEVEL 13: Multi-line While/Until ===
count=0
while [ $count -lt 5 ]; do
  echo "while: $count"
  count=$((count + 1))
done
echo "final: $count"
n=3
until [ $n -eq 0 ]; do
  echo "until: $n"
  n=$((n - 1))
done
echo "countdown done"
sum=0
i=1
while [ $i -le 10 ]; do
  sum=$((sum + i))
  i=$((i + 1))
done
echo "sum 1..10 = $sum"

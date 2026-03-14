echo === LEVEL 12: Multi-line For Loops ===
for fruit in apple banana cherry; do
  echo "fruit: $fruit"
done
for letter in a b c d; do
  UPPER=$(echo $letter | tr a-z A-Z)
  echo "$letter -> $UPPER"
done
for i in 1 2 3; do
  echo "item $i" > /tmp/ml_test_$i
done
for i in 1 2 3; do
  cat /tmp/ml_test_$i
done
rm /tmp/ml_test_1 /tmp/ml_test_2 /tmp/ml_test_3
echo "files cleaned"

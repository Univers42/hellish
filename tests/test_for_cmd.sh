X=$(echo abc | tr a-z A-Z)
echo "X=$X"
for letter in a b; do
  UPPER=$(echo $letter | tr a-z A-Z)
  echo "$letter -> $UPPER"
done

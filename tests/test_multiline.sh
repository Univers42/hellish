if true; then
  echo "multiline if works"
fi
if false; then
  echo "FAIL"
elif true; then
  echo "multiline elif works"
else
  echo "FAIL"
fi
for x in a b c; do
  echo "loop: $x"
done
x=0
while [ $x -lt 3 ]; do
  echo "while: $x"
  x=$((x+1))
done

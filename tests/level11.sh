echo === LEVEL 11: Multi-line If/Elif/Else ===
if true; then
  echo "simple if body"
fi
if false; then
  echo "FAIL"
else
  echo "else on new line"
fi
if false; then
  echo "FAIL"
elif true; then
  echo "elif on new line"
fi
if false; then
  echo "FAIL"
elif false; then
  echo "FAIL"
elif true; then
  echo "second elif works"
else
  echo "FAIL"
fi
X=42
if [ $X -gt 10 ]; then
  echo "X is big: $X"
fi

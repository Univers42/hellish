echo === LEVEL 16: Multi-line + Operators ===
if true; then
  echo "first"
fi && echo "and after if"
if false; then
  echo "FAIL"
fi || echo "or after failed if"
for x in a b; do
  echo $x
done ; echo "after for"
true && if true; then
  echo "AND before multi-line if"
fi
false || for v in yes; do
  echo "OR before for: $v"
done
if [ 1 -eq 1 ]; then
  echo "pass"
fi && if [ 2 -eq 2 ]; then
  echo "chained if"
fi

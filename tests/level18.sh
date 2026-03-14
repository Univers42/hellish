echo === LEVEL 18: Multi-line + Subshells ===
(
  echo "in subshell"
  X=inner
  echo "X=$X"
)
echo "after subshell: X=${X:-unset}"
for i in 1 2; do
  (
    echo "sub $i"
    Y=sub_$i
    echo "Y=$Y"
  )
done
if true; then
  (
    echo "subshell in if"
  )
fi
(
  for v in a b; do
    echo "sub-for: $v"
  done
)
echo "subshells done"

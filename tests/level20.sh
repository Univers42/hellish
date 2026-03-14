echo === LEVEL 20: Full Multi-line Integration ===
# Variable setup
STATUS=ok
COUNT=0
# Multi-line if with pipes
if [ "$STATUS" = "ok" ]; then
  echo "status check passed" | tr a-z A-Z
else
  echo "FAIL"
fi
# For loop writing and reading files
for name in alice bob charlie; do
  echo "user: $name" >> /tmp/ml_users
done
cat /tmp/ml_users | sort
rm /tmp/ml_users
# Nested loop with arithmetic
for i in 1 2 3; do
  val=$((i * i))
  if [ $val -gt 4 ]; then
    echo "$i^2=$val (big)"
  else
    echo "$i^2=$val"
  fi
done
# While + subshell + redirection
i=0
while [ $i -lt 3 ]; do
  (
    echo "sub[$i]: pid=child"
  )
  i=$((i+1))
done
# Complex chain
for x in hello goodbye; do
  echo $x
done | while read line; do
  echo "read: $line"
done
# Param expansion inside multi-line
GREETING="hello world"
for word in $GREETING; do
  echo "word: $word (len=${#word})"
done
# Final nested test
if true; then
  for n in 1 2; do
    if [ $n -eq 1 ]; then
      echo "first iteration" | cat
    else
      echo "last iteration" | tr a-z A-Z
    fi
  done && echo "loop chain ok"
fi
echo "=== ALL 20 LEVELS COMPLETE ==="

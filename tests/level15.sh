echo === LEVEL 15: Multi-line + Pipes and Redirections ===
for n in 1 2 3; do
  echo "piped: $n"
done | cat -n
if true; then
  echo "redirect test" > /tmp/ml_redir_test
  cat /tmp/ml_redir_test
  rm /tmp/ml_redir_test
fi
for word in HELLO WORLD; do
  echo $word | tr A-Z a-z
done
i=0
while [ $i -lt 3 ]; do
  echo "line $i"
  i=$((i+1))
done | sort -r
for f in alpha beta gamma; do
  echo "$f" >> /tmp/ml_append_test
done
cat /tmp/ml_append_test
rm /tmp/ml_append_test

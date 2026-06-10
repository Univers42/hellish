#!/bin/sh
# Here-document in a loop: each iteration feeds `read` from a fresh heredoc
# whose body expands the loop counter. Exercises per-iteration heredoc
# setup/teardown, the read builtin, and expansion inside heredoc bodies.
i=0
sum=0
while [ $i -lt 3000 ]; do
	read kind value <<EOF
item $i
EOF
	sum=$((sum + value))
	i=$((i+1))
done
echo "kind=$kind"
echo "last=$value"
echo "sum=$sum"

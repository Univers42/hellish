#!/bin/sh
# Here-docs: expanded, quoted (no expansion), and as stdin to a loop.
name=world
count=3

cat <<EOF
Hello, $name!
Count is $count and doubled is $((count * 2)).
EOF

echo "-- quoted delimiter (literal) --"
cat <<'EOF'
No expansion: $name $((1+1)) `echo hi`
EOF

echo "-- heredoc into while-read --"
while read -r a b; do
	echo "pair: $a / $b"
done <<EOF
1 one
2 two
3 three
EOF

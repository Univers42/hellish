echo === LEVEL 03: Pipes ===
echo hello | cat
echo "pipe chain" | cat | cat | cat
echo "hello world" | wc -w
echo -e "banana\napple\ncherry" | sort
echo -e "aaa\nbbb\naaa" | sort | uniq
echo "UPPER" | tr A-Z a-z
ls /tmp | head -3
echo "count:" | cat -e

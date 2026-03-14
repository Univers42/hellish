echo === LEVEL 04: Redirections ===
echo "write to file" > /tmp/test_redir_out
cat /tmp/test_redir_out
echo "append line" >> /tmp/test_redir_out
cat /tmp/test_redir_out
cat < /tmp/test_redir_out
echo "stderr test" 2>/dev/null
cat /nonexistent_file_42 2>/dev/null
echo "after failed cat: $?"
echo "overwrite" > /tmp/test_redir_out
cat /tmp/test_redir_out
rm /tmp/test_redir_out

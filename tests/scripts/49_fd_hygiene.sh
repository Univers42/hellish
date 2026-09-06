# Descriptors a command opened for its redirections must be closed in the
# parent as soon as that command is done, not at the end of the input cycle.
# configure's fcntl(F_DUPFD_CLOEXEC, 10) probe found fd 10 already taken
# after a few hundred `cmd 2>/dev/null` and answered "no" under hellish.
# Every count is relative to this script's own baseline so bash and hellish
# print the same lines.
n() { ls /proc/$$/fd | wc -l; }
base=$(n)
i=0; while [ $i -lt 60 ]; do /bin/true 2>/dev/null; i=$((i+1)); done
echo "after redirected externals: $(( $(n) - base ))"
i=0; while [ $i -lt 60 ]; do echo hi | cat >/dev/null; i=$((i+1)); done
echo "after pipelines: $(( $(n) - base ))"
i=0; while [ $i -lt 60 ]; do cat >/dev/null <<EOF
body $i
EOF
i=$((i+1)); done
echo "after heredocs: $(( $(n) - base ))"
i=0; while [ $i -lt 60 ]; do read -r l <<EOF
line $i
EOF
i=$((i+1)); done
echo "after builtin heredocs: $(( $(n) - base )) last=$l"
eval '/bin/true 2>/dev/null'; eval '/bin/true 2>/dev/null'; eval 'echo hi | cat >/dev/null'
echo "after evals: $(( $(n) - base ))"
f() { /bin/true 2>/dev/null; echo x >/dev/null; { echo y; } >/dev/null; }; f; f; f
echo "after functions: $(( $(n) - base ))"
exec 5>>/dev/null
exec 6>&1
i=0; while [ $i -lt 30 ]; do { echo x; } >&5; /bin/true 2>&5; (true) 2>&5; echo z >&6 >/dev/null; i=$((i+1)); done
echo "after configure-style fd 5/6 traffic: $(( $(n) - base - 2 ))"
echo "child fds at or above 10: $(ls /proc/self/fd | awk '$1 >= 10' | wc -l)"
/bin/sh -c 'ls /proc/self/fd' 2>/dev/null | awk '$1 >= 10 { c++ } END { print "grandchild fds at or above 10: " c+0 }'
# a persistent redirection must survive all of the above
echo "fd 5 still open: $(ls /proc/$$/fd | grep -c '^5$')"
echo "fd 6 still open: $(ls /proc/$$/fd | grep -c '^6$')"
# the heredoc loop must have delivered a fresh body each time
i=0; while [ $i -lt 3 ]; do cat <<EOF
iteration $i
EOF
i=$((i+1)); done
# `exec 10>f` parks the opened fd at 10, which IS the target: that entry must
# survive the command that installed it (2.9.1 closed it at the end of the
# input cycle, so the next line's `>&10` failed with EBADF).
exec 10>hb_ten
echo ten >&10
echo "fd 10 across lines: $(cat hb_ten)"
exec 10>&-
rm -f hb_ten

#!/bin/bash
# DEBUG trap granularity (issue #108, wave 3): bash treats an entire
# `.`/source run as ONE DEBUG event, while eval fires per inner
# statement (both measured on 5.3.9). hellish fired per sourced
# statement, so a theme rc's `trap hx_preexec DEBUG` ran its hook --
# and its $(date) fork -- before every line of each re-source: the
# "source ~/.hellishrc got slow again" report.
cat > dbg_inner.sh <<'EOF'
a=1
b=2
g(){ x=9; }
g
c=3
EOF
trap 'echo D' DEBUG
. ./dbg_inner.sh
echo "after-source"
eval 'p=1; q=2'
echo "after-eval"
trap - DEBUG
rm -f dbg_inner.sh
echo "done=$?"

#!/bin/bash
# Aliases and sourced files: bash makes an alias defined on line N of a
# sourced file visible from line N+1 of that same file (expansion happens
# per line read), but NOT later in the same line. hellish alias-spliced a
# sourced file once, up front, so a file that defines then uses an alias
# printed "command not found" (issue #105 family).
shopt -s expand_aliases 2>/dev/null || true

cat > sub_alias.sh <<'EOF'
alias greet='echo sourced-alias-ok'
greet
EOF
. ./sub_alias.sh
rm -f sub_alias.sh

# Same-line definition + use must NOT expand (bash parity: aliases are
# expanded when the line is read, before the definition executes).
cat > sub_alias2.sh <<'EOF'
alias late='echo late-expanded'; late
echo "same-line status: $?"
EOF
. ./sub_alias2.sh 2>/dev/null
rm -f sub_alias2.sh

# eval across a newline expands; pre-existing aliases inside eval expand.
alias pre='echo eval-pre-ok'
eval 'pre'
eval 'alias inner="echo eval-inner-ok"
inner'
unalias pre inner greet late 2>/dev/null
echo "done=$?"

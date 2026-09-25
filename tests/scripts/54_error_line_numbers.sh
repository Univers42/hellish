# A runtime error names the line it happened on -- and inside a sourced
# file, that file and its line.
#
# ctx, the "script: line N:" every runtime error starts with, was set only
# by the READER. Input arrives in batches -- a whole small script at once
# -- so by the time a command ran, the reader stood past the batch and every
# error named the line after the last one:
#
#     printf 'true\n./nope\n' > t.sh; hellish t.sh
#     t.sh: line 3: ./nope: No such file or directory      (bash: line 2)
#
# Inside a sourced file the error named the sourcing script, at a line of
# its own, and $LINENO was the `.` line's. Bash names the sourced file.
#
# Errors are folded into stdout here, so the corpus diff grades their text;
# only the script's own path is replaced, since the corpus hands the same
# path to both shells anyway. Missing commands are ./paths: their message
# is the same everywhere, unlike a PATH miss's.

T=$(mktemp -d) || exit 1
say() { sed "s|$0|SCRIPT|; s|$T|T|g"; }

{
	true
	./no-such-1
	echo "LINENO=$LINENO"
} 2>&1 | say

./no-such-2 2>&1 | say

cat > "$T/inc.sh" <<'EOF'
echo "inc LINENO=$LINENO"
./no-such-in-inc

f_in_inc() { echo "f_in_inc LINENO=$LINENO"; }
for i in 1 2; do
	./no-such-in-loop-$i
done
echo "inc end LINENO=$LINENO"
EOF

cat > "$T/outer.sh" <<'EOF'
./no-such-before-nested
. "$T_DIR/inc.sh"
./no-such-after-nested
EOF

T_DIR=$T
. "$T/inc.sh" 2>&1 | say
./no-such-after-source 2>&1 | say
. "$T/outer.sh" 2>&1 | say
./no-such-after-outer 2>&1 | say

# eval inside a sourced file reports from the eval's line in the file
cat > "$T/ev.sh" <<'EOF'
true
eval './no-such-in-eval'
EOF
. "$T/ev.sh" 2>&1 | say

# a heredoc before the error does not shift the count
cat > "$T/hd.sh" <<'EOF'
cat <<X
body line
X
./no-such-after-heredoc
EOF
. "$T/hd.sh" 2>&1 | say

# many lines of a big script: the count comes from the start, not a cache
i=0
while [ "$i" -lt 3 ]; do
	i=$((i + 1))
	./no-such-in-while-$i 2>&1 | say
done
echo "after while LINENO=$LINENO"

rm -rf "$T"

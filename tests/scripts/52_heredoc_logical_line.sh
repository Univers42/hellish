# Issue #139: `nvm install 22` printed "syntax error near unexpected token
# `newline'" twice and found no version. nvm quotes a multi-line awk program
# right before a `<<EOF`, inside $( ):
#
#     VERSIONS="$( { command awk '{
#           ...
#         }' \
#       | $SORT_COMMAND; } << EOF
#     $VERSION_LIST
#     EOF
#     )"
#
# Heredoc operators were found by lexing ONE PHYSICAL LINE at a time, so the
# line `}' | sort; } << EOF` read as an OPEN quote and the `<<` was never
# seen: the body ran as commands, and the fallback that went looking for it
# read past the end of the script buffer (an ASan heap-buffer-overflow).
# The same family: a `#` comment and a heredoc body inside $( ) were not
# known to the $( ) span scanner, and a heredoc in a one-line function body
# whose body followed the line was written into a redirect slot that was
# gone by the first call (SEGV).

# --- the nvm shape, distilled ------------------------------------------------
nvm_like() {
  local VERSION_LIST SORT_COMMAND PATTERN VERSIONS
  VERSION_LIST='v18.0.0	x	x	x	x	x	x	x	x	Hydrogen
v20.1.0	x	x	x	x	x	x	x	x	Iron
v22.3.0	x	x	x	x	x	x	x	x	Jod'
  SORT_COMMAND='command sort -t. -u -k 1.2,1n -k 2,2n -k 3,3n'
  PATTERN='v22'
  VERSIONS="$( { command awk -v lts="" '{
        if (!$1) { next }
        if (lts && $10 ~ /^\-?$/) { next }
        if ($10 !~ /^\-?$/) {
          print $1, $10
        } else {
          print $1
        }
      }' \
    | grep -w "${PATTERN:-.*}" \
    | $SORT_COMMAND; } << EOF
$VERSION_LIST
EOF
)"
  echo "versions=[$VERSIONS]"
}
nvm_like

# --- a newline inside a lexeme is not the end of the command line ---------------
V=$(echo 'a
b'; cat << EOF
sq
EOF
); echo "1:[$V]"
V=$(echo "a
b"; cat << EOF
dq
EOF
); echo "2:[$V]"
V=`echo 'a
b'; cat << EOF
bq
EOF
`; echo "3:[$V]"
V=$(echo $'a
b' | cat << EOF
pipe
EOF
); echo "4:[$V]"
cat <<A; echo "x
y"
top body
A
{ echo 'grp
ok'; cat; } << EOF
group body
EOF

# --- comments inside $( ) -----------------------------------------------------
x=$(echo a # it's a comment
); echo "5:[$x]"
x=$(# (
echo ok); echo "6:[$x]"
x=$(echo a;#)
echo b); echo "7:[$x]"
x=$(echo a#b $(echo c)#d "e"#f); echo "8:[$x]"

# --- heredoc bodies inside $( ) are not shell text ------------------------------
x=$(cat <<EOF
it's
EOF
); echo "9:[$x]"
x=$(cat <<EOF
a ) b
EOF
); echo "10:[$x]"
x=$(cat <<'E F'
a ) b " $HOME
E F
); echo "11:[$x]"
x=$(cat <<-EOF
	tab ) stripped
	EOF
); echo "12:[$x]"
x=$(cat <<A <<B
first (
A
second '
B
); echo "13:[$x]"
x=$(case y in y) cat <<EOF
in case )
EOF
;; esac); echo "14:[$x]"

# --- ...but a shift is not a heredoc ------------------------------------------
x=$(
echo $((1<<2))
echo done
); echo "15:[$x]"
x=$(
(( y = 1 << 3 ))
echo $y
); echo "16:[$x]"
x=$(cat <<<"here string"
echo after
); echo "17:[$x]"

# --- a function body's heredoc lives with the function --------------------------
f1() { cat <<EOF; }
f1 body $((6*7))
EOF
f1; f1
f2() { echo pipe | cat <<EOF; }
f2 body
EOF
f2
f3() { cat <<-EOF; }
	f3 body
	EOF
f3
f4() { cat <<'EOF'; }
$HOME stays literal
EOF
f4
f5() { cat <<A; cat <<B; }
a
A
b
B
f5
for i in 1 2; do cat <<EOF; done
loop $i
EOF

# --- eval and source: a chunk never ends inside a backslash-newline join ------
# nvm is SOURCED, and source runs text in chunks; growing a chunk to the line
# before `| sort; } << EOF` cut it between a `\` and its continuation, and
# `{ ... \` then parsed as an error instead of "needs more".
cat > sourced_nvm_shape.sh <<'SRC'
g() {
  L='v1 x
v2 y'
  V="$( { command awk '{
        print $1
      }' \
    | grep v2 \
    | cat; } << EOF
$L
EOF
)"
  echo "sourced=[$V]"
}
SRC
. ./sourced_nvm_shape.sh
g
rm -f sourced_nvm_shape.sh
eval "{ echo 'e
val' \\
  | cat \\
  | cat; } << EOF
x
EOF"

# --- eval and source read their own text, not the caller's --------------------
eval 'cat <<EOF' 2>/dev/null
eval-body
EOF
echo "after eval"

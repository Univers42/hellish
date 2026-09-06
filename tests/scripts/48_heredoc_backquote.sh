# Backquoted command substitution inside an unquoted here-document body.
# autoconf writes its confdefs.h this way; a shell that keeps the backquotes
# literal makes every later header check fail to compile.
as_echo='printf %s\n'
as_tr_cpp="eval sed 'y%*abcdefghijklmnopqrstuvwxyz%PABCDEFGHIJKLMNOPQRSTUVWXYZ%;s%[^_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]%_%g'"
for ac_header in sys/types.h sys/stat.h stdlib.h; do
	cat >>confdefs.h <<_ACEOF
#define `$as_echo "HAVE_$ac_header" | $as_tr_cpp` 1
_ACEOF
done
cat confdefs.h
rm -f confdefs.h

x=42
f() { echo "fn:$1"; }
cat << EOF
one: `echo hi`
two: `echo a` `echo b`
three: `echo \$x` `f arg`
four: $(echo dollar) `echo bq` $((x + 1))
five: `printf '%s\n' 'quoted inside'`
EOF

cat << 'EOF'
quoted delimiter keeps `echo this` literal
EOF

cat <<- EOF
	tab: `echo stripped`
	EOF
echo "status=$?"

# A lone $ (no name after it) must stay a $ and must not eat the next byte:
# config.status's awk program is written from a heredoc as `line = $ 0`, and
# a shell that turns that into `line =   0` prints "0" for every Makefile line.
cat << EOF
  line = $ 0
  n = split(line, field, "@")
  z = $
  w = $%
  price: 5$ each
EOF

#!/bin/sh
# Heredoc bodies expand in DOUBLE-QUOTE context: a single quote inside a
# ${...} operator word is an ordinary character there, NOT a quoting
# operator, so $var inside it still expands and the quotes survive into the
# output. hellish used to expand heredoc ${} words with unquoted semantics,
# which stripped the quotes and suppressed the expansion:
#   ${foo:+'blah  $foo'}  printed  blah  $foo   (wrong)
#   bash and dash both print  'blah  1'
# Also pins the contrasting contexts, which were already correct and must
# stay that way: inside real double quotes the quotes are literal too, while
# in a plain unquoted word single quotes DO quote.

foo=1

# --- heredoc: quotes literal, $foo expands ---
cat <<EOM
${foo:+'blah  $foo'}
EOM

cat <<EOM
${foo:-'x $foo'}
EOM

# unset branch: the :- word is used, quotes still literal
empty=
cat <<EOM
${empty:+'a $foo'}${empty:-'b $foo'}
EOM

# nested braces inside the heredoc word
cat <<EOM
${foo:+'v=${foo}'}
EOM

# --- contrast: real double quotes behave the same way ---
echo "${foo:+'blah $foo'}"
echo "${foo:-'x $foo'}"

# --- contrast: unquoted word -- here single quotes DO quote ---
echo ${foo:+'blah $foo'}
echo ${foo:-'x $foo'}

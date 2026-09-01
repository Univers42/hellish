#!/bin/bash
# Extended glob patterns on the right of == / != inside [[ ]] work in
# bash WITHOUT `shopt -s extglob`: the option is temporarily enabled
# while the operand is parsed and matched (bash 4.1+). hellish's [[ ]]
# matcher rejected them with "test: syntax error" (issue #105 family —
# bash-completion's _rl_enabled does exactly this on its first screen).
x="a  on"
[[ $x == *+([[:space:]])on* ]] && echo "space-class: matched"
[[ "abc" == a@(b|z)c ]] && echo "at-group: matched"
[[ "abc" == a+(b)c ]] && echo "plus-group: matched"
[[ "ac" == a?(b)c ]] && echo "opt-group: matched"
[[ "axc" == a!(b)c ]] && echo "negate-group: matched"
[[ "abbc" == a*(b)c ]] && echo "star-group: matched"
[[ "abc" != a@(x|y)c ]] && echo "negative-match: ok"
v="theme+plain"
[[ $v == *+* ]] && echo "literal-plus-still-glob: ok"
echo "done=$?"

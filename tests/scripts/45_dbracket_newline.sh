#!/bin/bash
# Newlines inside [[ ]] where bash's conditional grammar tolerates them:
# after the [[ itself, after && and ||, and before ]]. bash-completion
# 2.16 wraps its compat-dir test as `[[ a != @(...) &&\n -f b ]]`, and
# emitting that newline as a token cut the conditional in half — once per
# drop-in file at every Debian 13 login (issue #105, wave 2).
x=abc
[[ $x == a* &&
    -n $x ]] && echo "and-continuation: ok"
[[ $x == zz* ||
    -n $x ]] && echo "or-continuation: ok"
[[ -n $x
]] && echo "before-close: ok"
[[
-n $x ]] && echo "after-open: ok"
[[ $x == a* &&
   $x == *c &&
   -n $x ]] && echo "chained: ok"
y="cat li"
w=cat
[[ ${y:0:${#w}} == "$w" ]] && echo "nested-substring-bound: ok"
echo "done=$?"

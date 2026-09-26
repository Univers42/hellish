# ${var@P}: the value rendered as a prompt string (bash 4.4). This is the
# part hellish and bash agree on byte for byte -- the escapes both have,
# parameter and arithmetic expansion, and that what a variable expanded
# to is NOT read again for escapes. hellish's own additions (the zsh `%`
# escapes of its bilingual PS1, \A, \g) and `${PS1@P}` being exactly the
# live prompt are pinned at a real prompt, tests/prompt_expand_p_test.py.
d=$(mktemp -d)
cd "$d" || exit 1
mkdir leaf && cd leaf || exit 1
HOME=$d

show() { for a; do printf '<%s>\n' "$a"; done; }

P='\w \W \$ \\ \n.'
show "${P@P}"
P='\u' Q='\h' R='\H'
[ "${P@P}" = "$(id -un)" ] && echo "\\u is the user"
[ "${Q@P}" = "$(hostname | cut -d. -f1)" ] && echo "\\h is the host"
[ "${R@P}" = "$(hostname)" ] && echo "\\H is the full host"

# \[ \] are readline's markers; with no line editor they are nothing.
P='a\[b\]c\e[0m'
show "${P@P}" | od -An -c | tr -s ' '

# \nnn octal, \D{fmt}.
P='\101\060 \D{%Y}'
[ "${P@P}" = "A0 $(date +%Y)" ] && echo "octal and \\D"

# Expansions run after the escapes, and what they produce stays as it is.
V='100%done \w %d \$HOME $HOME'
P='[${V}] [$V] [$((6 * 7))] [${V%% *}] [${nosuch-unset}]'
show "${P@P}"

# An empty and an unset variable both render as nothing.
E=
show "${E@P}" "${nosuch@P}"

# Positionals and a function's arguments.
set -- '\W' '$((1+1))'
show "${1@P}" "${2@P}"
f() { show "${1@P}"; }
f 'in \W'

# In double quotes, in an assignment, as part of a word.
P='\W'
x=${P@P}
show "$x" "pre${P@P}post"

cd / && rm -rf "$d"

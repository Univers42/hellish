# A quoted "$@" or "${a[@]}" expands to one field per element, and quoting
# means no pathname expansion: an element that is `*` stays `*`.
#
# hellish globbed every element, because the fields were built as unquoted
# expansion tokens. `f() { printf '[%s]' "$@"; }; f '*'` printed every file
# in the directory, and any wrapper of the form `rm -- "$@"` deleted them.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work" || exit 1
: > aa; : > ab; : > b1; mkdir dd

show() { printf '[%s]' "$@"; echo; }

echo "-- positionals"
set -- '*' 'a?' '[ab]*' 'dd/*' '' 'x y'
show "$@"
show "${@}"
show "${@:1}"
show "${@:2:2}"
for i in "$@"; do printf '<%s>' "$i"; done; echo
show pre"$@"post
show "x$@"
show "$@y"
count() { echo "$#"; }
count "$@"

echo "-- a function's own arguments"
fwd() { show "$@"; }
fwd '*' '?b'
wrap() { local a="$1"; shift; show "$a" "$@"; }
wrap '*' '*' 'a*'

echo "-- arrays"
a=('*' 'a?' '' '[ab]*')
show "${a[@]}"
for e in "${a[@]}"; do printf '<%s>' "$e"; done; echo
show "p${a[@]}s"

echo "-- associative arrays"
declare -A h
h['*']='a*'
show "${h[@]}"
show "${!h[@]}"

echo "-- still unquoted, still globbed"
set -- '*' 'b?'
show $@
show ${a[0]}
b=('a*')
show ${b[@]}

echo "-- set -f turns globbing off for both"
set -f
show $@
set +f

echo "-- a quoted element next to an unquoted glob: only the glob matches"
: > 'a*z'
set -- a '*' 'a*'
show "$@"*
show *"$@"
show "${@:2}"*
c=(a 'a*' b)
show "${c[@]}"*
rm -f 'a*z'

echo "-- the destructive case"
del() { rm -f -- "$@"; }
del '*'
ls

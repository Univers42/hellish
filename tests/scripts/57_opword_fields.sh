# An unquoted ${p-w} / ${p:-w} / ${p+w} / ${p:+w} / ${p=w} / ${p?w} is
# field-split and globbed like any unquoted expansion. The variable's own
# value is split like $p. A used word is split and globbed where the word
# was unquoted, and kept whole where it was quoted.
#
# hellish kept the whole result as one field whenever the word's TEXT had
# no unquoted blank: P='a b c'; set -- ${P:-x} gave $# = 1, and
# ${u:-$P}, ${u:-*.c} were never split or globbed.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work" || exit 1
: > a1; : > a2

n() { printf '%s:' "$#"; printf '[%s]' "$@"; echo; unset U; }
P="a b c"; Q="a*"; unset U

echo "-- the variable's own value"
n ${P:-x}
n ${P-x}
n ${P:-"c d"}
n ${P:?unset}
n ${P:+$P}
n ${P+$P}

echo "-- a used word: unquoted parts split and glob"
n ${U:-$P}
n ${U-$P}
n ${U:-a b}
n ${U:-$Q}
n ${U:-a*}
n ${U:-$(echo "a  b")}
n ${U:-`echo a b`}
P1=1
n ${P1:+$Q}
n ${P1:+a  b}

echo "-- quoted parts stay whole and literal"
n ${U:-"c d"}
n ${U:-'a b'}
n ${U:-a\ b}
n ${U:-\*}
n ${U:-"*"}
n ${U:-a"*"}
n ${U:-"$P"}
n ${U:-"$Q"}
n ${P1:+"$Q"}
n ${U:-"a"'b'$'\t'c}

echo "-- mixed words"
n ${U:-"x"$P}
n ${U:-$P"$P"}
n ${U:-"a b"c d}
n ${U:-$Q"x"}

echo "-- empty results"
n ${U:-}
n ${U:-$E}
n ${U:-""}
n ${U:-''}
n ${U+x}
n pre${U:-}post
n pre${U:-" "}post
d=; a=()
n x ${a:+"${a[@]}"} ${d:+--git-dir="$d"} y
d='p q'
n ${d:+--git-dir="$d"}

echo "-- assignment: the new value, split like the variable"
n ${U:="a b"}
n ${U:=a b}
n ${U:=$Q}
n ${U:=\*}

echo "-- a tilde prefix is quoted, the rest of the word is not"
H0=$HOME; HOME="/a b"
n ${U:-~}
n ${U:-~/x}
n ${U:-~/x y}
n ${U:=~}
HOME=$H0

echo "-- positionals in the word"
set -- 'x y' z
n ${U:-"$@"}
n ${U:-$@}
n ${U:-$*}

echo "-- IFS, set -f, and the contexts that do not split"
IFS=:; Z='a:b c'
n ${U:-$Z}
n ${U:-"$Z"}
unset IFS
set -f
n ${U:-a*}
set +f
n "${U:-a*}" "${U:-$P}"
v=${U:-$P}
n "$v"
case a in ${U:-a}) echo "case: matched" ;; esac
for i in ${U:-1 2 "3 4"}; do printf '<%s>' "$i"; done; echo

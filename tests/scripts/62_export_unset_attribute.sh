# `export NAME` before NAME has a value. POSIX: the export attribute is
# set, and "subsequent" assignments are exported. Until one comes, NAME
# is still unset: no child sees it, ${NAME+x} is empty, `set` leaves it
# out, and `export -p` / `declare -p` list it without a value.

# The idiom itself.
export A
A=1
sh -c 'echo "A in child: [$A]"'

# Before it is assigned: unset everywhere, listed bare.
export B
echo "B set: [${B+yes}]"
env | grep -c '^B=' || true
export -p | grep -E '^export B($|=)'
declare -p B
set | grep -c '^B=' || true
( set -u; : "$B" ) 2>/dev/null || echo "B unbound under set -u"

# Any way of assigning it exports it.
export C D E F
read -r C <<EOF
from-read
EOF
for D in from-for; do :; done
printf -v E '%s' from-printf 2>/dev/null || E=from-printf
F+=appended
sh -c 'echo "C=[$C] D=[$D] E=[$E] F=[$F]"'

# declare -x does the same.
declare -x G
G=2
sh -c 'echo "G in child: [$G]"'
declare -p G

# unset takes the attribute away with the variable.
export H
H=3
unset H
H=4
sh -c 'echo "H after unset: [${H-unset}]"'

# export -n and declare +x take it away and leave the name unset.
export I
export -n I
I=5
sh -c 'echo "I after export -n: [${I-unset}]"'
export J
declare +x J
J=6
sh -c 'echo "J after declare +x: [${J-unset}]"'

# export -p output can be fed back to a shell and means the same thing.
export K
L=val
export L
export -p | grep -E '^export (K|L)($|=)' > "${TMPDIR:-/tmp}/exp_$$"
unset K L
. "${TMPDIR:-/tmp}/exp_$$"
rm -f "${TMPDIR:-/tmp}/exp_$$"
echo "K set: [${K+yes}] L=[$L]"
K=k
sh -c 'echo "K=[$K] L=[$L]"'

# Exporting a name that is already set changes nothing but the attribute.
M=keep
export M
sh -c 'echo "M=[$M]"'

# cd with PWD exported but unset: OLDPWD takes PWD's state, attribute
# included, and PWD is set again.
(
	unset PWD
	export PWD
	cd /
	echo "OLDPWD set: [${OLDPWD+yes}]"
	export -p | grep -E '^export OLDPWD($|=)'
	echo "PWD=[$PWD]"
)

# A readonly name exported before it has a value stays readonly.
(
	export N
	readonly N
	declare -p N
	N=1
) 2>/dev/null
echo "readonly N: status $?"

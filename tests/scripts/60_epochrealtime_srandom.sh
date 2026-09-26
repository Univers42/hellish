# $EPOCHREALTIME and $SRANDOM (bash 5.1). Their values change on every
# read, so this prints properties, never the values: the format, where
# they sit relative to $EPOCHSECONDS, that they move, and that $SRANDOM
# has 32 bits and cannot be seeded through $RANDOM.

echo "set: ${EPOCHREALTIME:+yes} ${SRANDOM:+yes}"

# EPOCHREALTIME: seconds, a '.', exactly six digits of microseconds.
r=$EPOCHREALTIME
case $r in
	*[!0-9.]* | *.*.* | .* | *.) echo "realtime: bad format" ;;
	*.[0-9][0-9][0-9][0-9][0-9][0-9]) echo "realtime: sec.usec" ;;
	*) echo "realtime: bad format" ;;
esac

# Its seconds are EPOCHSECONDS read just before and just after.
s1=$EPOCHSECONDS
r=$EPOCHREALTIME
s2=$EPOCHSECONDS
if [ "${r%.*}" -ge "$s1" ] && [ "${r%.*}" -le "$s2" ]; then
	echo "realtime: agrees with EPOCHSECONDS"
fi

# It moves forward, in microseconds.
us() { echo $(( ${1%.*} * 1000000 + 10#${1#*.} )); }
a=$EPOCHREALTIME
sleep 0.05
b=$EPOCHREALTIME
d=$(( $(us "$b") - $(us "$a") ))
if [ "$d" -ge 40000 ] && [ "$d" -lt 5000000 ]; then
	echo "realtime: moved about 50ms"
else
	echo "realtime: moved $d us"
fi

# SRANDOM: a decimal number in 0 .. 2^32-1, new on every read.
bad=0 big=0 same=0 i=0 prev=
while [ "$i" -lt 16 ]; do
	v=$SRANDOM
	case $v in '' | *[!0-9]*) bad=1 ;; esac
	[ "${#v}" -le 10 ] && [ "$v" -le 4294967295 ] || bad=1
	[ "$v" -gt 32767 ] && big=$((big + 1))
	[ "$v" = "$prev" ] && same=$((same + 1))
	prev=$v
	i=$((i + 1))
done
echo "srandom: bad=$bad repeats=$same wider-than-RANDOM=$([ "$big" -gt 0 ] && echo yes)"

# Seeding $RANDOM never makes $SRANDOM repeat. (Whether RANDOM=42 reseeds
# $RANDOM itself is not asserted: hellish does not, a documented
# divergence in expand2.c.)
RANDOM=42; x1=$SRANDOM
RANDOM=42; x2=$SRANDOM
[ "$x1" != "$x2" ] && echo "RANDOM=42 does not repeat SRANDOM"

# A subshell and a command substitution draw fresh values.
p=$SRANDOM
c=$(echo "$SRANDOM")
[ "$p" != "$c" ] && echo "srandom: fresh in a command substitution"

# Arithmetic reads them like any variable.
echo "arith: $(( SRANDOM >= 0 && SRANDOM <= 4294967295 ))"
echo "arith: $(( ${EPOCHREALTIME%.*} >= s1 ))"

# Assigning does not make them constant.
SRANDOM=7
t=0; for i in 1 2 3; do [ "$SRANDOM" = 7 ] && t=$((t + 1)); done
echo "SRANDOM=7 ignored: $([ "$t" -lt 3 ] && echo yes)"
EPOCHREALTIME=1.000000
case $EPOCHREALTIME in 1.000000) echo "EPOCHREALTIME=1 stuck" ;;
	*) echo "EPOCHREALTIME=1 ignored" ;; esac

# set -u: both are always set.
( set -u; : "$EPOCHREALTIME" "$SRANDOM"; echo "nounset: ok" )

#!/bin/bash
# gen_combos.sh — generate a large battery of test cases covering the constructs
# hardened this cycle (heredocs, ${VAR} forms, [[ ]] logic, brace, line-cont) and
# their combinations, KEEPING ONLY cases where hellish output+exit == bash --posix.
# Output: tests/regress_combos (consumed by the tester). Run from tests/.
set -u
H="$(cd .. && pwd)/build/bin/hellish"
OUT="regress_combos"
WORK="$(mktemp -d /tmp/gencombos_XXXXXX)"
: > "$OUT"
pass=0; fail=0

filt() { perl -pe 's/\e\[[0-9;]*[A-Za-z]//g' | grep -vE '^[[:space:]]*❯' | grep -v '^exit$'; }

emit() {
	local t="$1" mo mb me be
	mo=$(cd "$WORK" && ASAN_OPTIONS=detect_leaks=0 timeout 5 "$H" -c "$t" 2>/dev/null | filt); me=${PIPESTATUS[0]}
	mb=$(cd "$WORK" && timeout 5 bash --posix -c "$t" 2>/dev/null | filt); be=${PIPESTATUS[0]}
	if [ "$mo" = "$mb" ] && [ "$me" = "$be" ]; then
		printf '%s\n' "$t" >> "$OUT"; pass=$((pass+1))
	else
		fail=$((fail+1)); printf '%s\n' "$t" >> "$WORK/failed.txt"
	fi
}

# --- [[ ]] string comparison: operands x operators -------------------------
for a in a b abc "" "a b" xyz; do
	for b in a b abc "" "a b" xyz; do
		for op in = == != ; do
			emit "[[ \"$a\" $op \"$b\" ]] && echo T || echo F"
		done
	done
done

# --- [[ ]] integer comparison ----------------------------------------------
for x in 0 1 5 -3 100 42; do
	for y in 0 1 5 -3 100 42; do
		for op in -eq -ne -lt -gt -le -ge; do
			emit "[[ $x $op $y ]] && echo T || echo F"
		done
	done
done

# --- [[ ]] logic: &&, ||, !, ( ) precedence + short-circuit ----------------
for p in '1 == 1' '1 == 2' '-n x' '-z ""' '5 -gt 3' '5 -lt 3'; do
	for q in '2 == 2' '2 == 3' '-n y' '-z ""' '9 -ge 9' '1 -ge 9'; do
		emit "[[ $p && $q ]] && echo AND || echo no"
		emit "[[ $p || $q ]] && echo OR || echo no"
		emit "[[ ! ( $p ) && $q ]] && echo NG || echo no"
		emit "[[ ( $p || $q ) && ( $p && $q ) ]] && echo GRP || echo no"
	done
done
emit '[[ 1 == 1 && 2 == 2 && 3 == 3 ]] && echo chain3'
emit '[[ 1 == 2 || 2 == 3 || 3 == 3 ]] && echo orchain'
emit '[[ ! 1 == 2 ]] && echo neg'
emit '[[ ! ! 1 == 1 ]] && echo dneg'
emit '[[ ( ( 1 == 1 ) ) ]] && echo nestgrp'
emit '[[ -n abc && ! -z abc ]] && echo bothnz'

# --- [[ ]] file/unary tests ------------------------------------------------
for f in /etc /etc/hosts /nonesuch /bin/sh /; do
	for op in -e -f -d -r -s -L; do
		emit "[[ $op $f ]] && echo Y || echo N"
	done
done
for s in "" x "a b" 0; do
	emit "[[ -z \"$s\" ]] && echo Z || echo NZ"
	emit "[[ -n \"$s\" ]] && echo NN || echo EE"
done

# --- [ ] / test with == (now accepted) -------------------------------------
emit '[ a == a ] && echo bracket_eq'
emit 'test abc == abc && echo test_eq'
emit '[ a == b ] || echo bracket_ne'
emit '[ 5 -gt 3 ] && echo bint'

# --- parameter expansion forms ---------------------------------------------
for v in hello "" "a/b/c" "x.y.z" "  pad  "; do
	emit "v='$v'; echo \"[\${v}]\""
	emit "v='$v'; echo \"len=\${#v}\""
	emit "v='$v'; echo \"\${v:-DEF}\""
	emit "v='$v'; echo \"\${v:+SET}\""
	emit "v='$v'; echo \"\${v#*/}\""
	emit "v='$v'; echo \"\${v##*/}\""
	emit "v='$v'; echo \"\${v%/*}\""
	emit "v='$v'; echo \"\${v%%/*}\""
done
emit 'echo "${undef:-fallback}"'
emit 'echo "${undef:=now}"; echo "$undef"'
emit 'unset u; echo "${u:+x}"; u=1; echo "${u:+x}"'
emit 'v=aXbXc; echo "${v/X/-}"; echo "${v//X/-}"'
emit 'v=hello; echo "${v/l/L}"'
emit 'set -- one two three; echo "$#"; echo "$1-$2-$3"; echo "$*"'
emit 'set -- a b c; for x in "$@"; do echo "<$x>"; done'

# --- arithmetic ------------------------------------------------------------
for e in '1+2' '10-3' '4*5' '20/3' '20%3' '2**8' '(1+2)*3' '7&3' '7|8' '5^1' \
	'1<<4' '256>>2' '5>3' '3>=3' '2<1' '1==1' '1!=2' '!0' '~0' '-5+3' \
	'1&&1' '0||1' '3>2?10:20' 'a=5,a*2'; do
	emit "echo \$(( $e ))"
done
emit 'a=5; echo $((a++)); echo $a; echo $((++a))'
emit 'a=2; a+=3; echo $a 2>/dev/null || echo $((a=a))'
emit 'i=0; while [ $i -lt 5 ]; do printf "%d" $i; i=$((i+1)); done; echo'

# --- brace expansion -------------------------------------------------------
emit 'echo {a,b,c}'
emit 'echo x{a,b}y'
emit 'echo {a,b}{c,d}'
emit 'echo pre{1,2,3}post'
emit 'echo {a,b,c}.txt'
emit 'echo "{a,b}"'
emit "echo '{a,b}'"
emit 'v=p; echo $v{1,2,3}'
emit 'echo a{b}c'
emit 'echo {}'
emit 'echo {a}'

# --- heredocs (printf-source single-liners: bodies use \n; cat to stdout) --
emit "printf 'cat <<EOF\nplain line\nEOF\n' > h.sh; . ./h.sh"
emit "v=world; printf 'cat <<EOF\nhello \$v\nbraced \${v}\nEOF\n' > h.sh; . ./h.sh"
emit "v=world; printf 'cat <<\"EOF\"\nno \$v\nno \${v}\nEOF\n' > h.sh; . ./h.sh"
emit "printf 'cat <<EOF\na\nEOF\ncat <<EOF\nb\nEOF\n' > h.sh; . ./h.sh"
emit "printf 'cat <<A; cat <<B\nfirst\nA\nsecond\nB\n' > h.sh; . ./h.sh"
emit "printf 'f(){ cat <<EOF\ninfunc \${1}\nEOF\n}\nf ARG\n' > h.sh; . ./h.sh"
emit "v=6.6.32; printf 'cat <<EOF\nver=\${v}-x\nv2=\$v\nEOF\n' > h.sh; . ./h.sh"
emit "printf 'cat <<EOF\n\$(( 2 + 3 ))\n\${HOME:+yes}\nEOF\n' > h.sh; . ./h.sh"
emit "printf 'cat <<-EOF\n\tstripped\nEOF\n' > h.sh; . ./h.sh"
emit "n=hi; printf 'cat <<EOF\na\${n}b\n\${n}\${n}\nEOF\n' > h.sh; . ./h.sh"
emit "printf 'cat <<EOF\n\${undef:-D}\n\${#n}\nEOF\n' > h.sh; n=abc; . ./h.sh"
emit "printf 'read x <<EOF\nfromhd\nEOF\necho got=\$x\n' > h.sh; . ./h.sh"
emit "printf 'cat <<E1\nbody1\nE1\ncat <<E2\nbody2\nE2\ncat <<E3\nbody3\nE3\n' > h.sh; . ./h.sh"

# --- line continuation (printf-source so the \\<newline> is real) ----------
emit "printf 'echo a \\\\\nb \\\\\nc\n' > c.sh; . ./c.sh"
emit "printf 'for i in 1 \\\\\n2 \\\\\n3; do printf \"%%s\" \"\$i\"; done; echo\n' > c.sh; . ./c.sh"
emit "printf 'if true && \\\\\ntrue; then echo cont_if; fi\n' > c.sh; . ./c.sh"
emit "printf 'g(){ echo p \\\\\nq \\\\\nr; }\ng\n' > c.sh; . ./c.sh"
emit "printf 'x=\$((1 + \\\\\n2 + \\\\\n3)); echo \$x\n' > c.sh; . ./c.sh"

# --- quoting / escaping ----------------------------------------------------
emit "echo 'single quoted'"
emit 'echo "double quoted"'
emit 'echo "a\"b"'
emit "echo 'a'\\''b'"
emit 'echo "tab	here"'
emit 'v=x; echo "$v"'"'"'$v'"'"
emit 'echo "$( echo nested )"'
emit 'echo `echo backtick`'
emit 'echo a"b"c'"'"'d'"'"'e'
emit 'printf "%s\n" "one two" three'

# --- control flow ----------------------------------------------------------
emit 'for i in a b c; do echo $i; done'
emit 'i=0; while [ $i -lt 3 ]; do echo $i; i=$((i+1)); done'
emit 'i=0; until [ $i -ge 3 ]; do echo $i; i=$((i+1)); done'
emit 'if [ 1 -eq 1 ]; then echo yes; else echo no; fi'
emit 'if [ 1 -eq 2 ]; then echo a; elif [ 2 -eq 2 ]; then echo b; else echo c; fi'
emit 'case xyz in x*) echo star;; *) echo other;; esac'
emit 'case abc in a) echo a;; abc) echo full;; esac'
emit 'for x in {1,2,3}; do echo n$x; done'
emit 'f(){ echo "args=$#"; for a in "$@"; do echo "$a"; done; }; f one two'

# --- combinations: [[ ]] inside if/while/&&, with expansions ---------------
emit 'v=foo; if [[ $v == foo && -n $v ]]; then echo match; fi'
emit 'v=5; if [[ $v -gt 3 && $v -lt 10 ]]; then echo inrange; fi'
emit 'for v in a b c; do [[ $v == b ]] && echo "found $v"; done'
emit 'x=1; [[ $x == 1 ]] && [[ $x -eq 1 ]] && echo both'
emit 'v=hello; [[ ${#v} -eq 5 && $v == hello ]] && echo lenmatch'
emit 'n=0; while [[ $n -lt 3 ]]; do echo $n; n=$((n+1)); done'
emit 'v=""; [[ -z $v || $v == x ]] && echo emptyok'
emit 'a=1; b=2; [[ ( $a -eq 1 || $b -eq 9 ) && $b -eq 2 ]] && echo grpcombo'

# --- pipes / redirection / cmdsub ------------------------------------------
emit 'echo hello | cat'
emit 'echo a; echo b | tr a-z A-Z'
emit 'printf "%s\n" c b a | sort'
emit 'echo $(echo nested $(echo deep))'
emit 'x=$(printf "1\n2\n3"); echo "$x" | wc -l'
emit 'echo one two three | wc -w'
emit 'true && echo t || echo f; false && echo t || echo f'

# --- extra coverage batch -------------------------------------------------
# more arithmetic
for e in '0x1f' '010' '1+2*3-4' '(5+5)/2' '100%7' '2*2*2*2' '15&9' '15|16' \
	'12^10' '1<<8' '1024>>3' '-(-5)' '+7' '0==0' '5%%2' '3*3>8' '10/0' \
	'1?2?3:4:5' 'a=3;b=4;a*b' '1+1==2' '5>3&&2<4' '0||0||1'; do
	emit "echo \$(( $e )) 2>/dev/null; true"
done
emit 'echo $((2#1010))'
emit 'echo $((8#17))'
emit 'echo $((16#ff))'
emit 'x=10; echo $((x*x)); echo $((x-=3)); echo $x'
emit 'echo $(( 3 > 2 ? 100 : 200 ))'

# case with alternations and globs
emit 'case abc in a*|b*) echo ab;; *) echo other;; esac'
emit 'case foo in bar|foo|baz) echo matched;; esac'
emit 'case "" in "") echo empty;; *) echo nonempty;; esac'
emit 'case file.txt in *.txt) echo text;; *.md) echo md;; esac'
emit 'for f in one.c two.h three.c; do case $f in *.c) echo "C: $f";; esac; done'
emit 'case 5 in [0-9]) echo digit;; *) echo no;; esac'
emit 'case ABC in [A-Z]*) echo upper;; esac'

# nested command substitution + expansions
emit 'echo $(echo $(echo $(echo deep)))'
emit 'x=$(echo hi); y=$(echo "$x there"); echo "$y"'
emit 'echo "count: $(echo a b c d | wc -w)"'
emit 'v=$(printf "%s-%s" a b); echo "$v"'
emit 'echo "$(date +x 2>/dev/null >/dev/null; echo ok)"'
emit 'n=3; echo "$(for i in $(seq 1 $n 2>/dev/null || echo 1 2 3); do printf x; done)"'

# printf format coverage
emit 'printf "%s|%s|%s\n" a b c'
emit 'printf "%d-%d\n" 7 42'
emit 'printf "%05d\n" 42'
emit 'printf "%c%c%c\n" h i 33'
emit 'printf "%%literal%%\n"'
emit 'printf "%s\n" a b c d'
emit 'printf "[%5s]\n" hi'
emit 'printf "[%-5s]\n" hi'
emit 'printf "%x %o\n" 255 8'

# parameter / positional combos
emit 'set -- a b c d e; echo "$#"; shift 2; echo "$#"; echo "$1"'
emit 'set -- x y z; echo "${1}${2}${3}"'
emit 'f(){ echo "${1:-none}"; }; f; f given'
emit 'v=path/to/file.tar.gz; echo "${v##*.}"; echo "${v%%.*}"; echo "${v%.*}"'
emit 'v=HELLO; echo "${#v}"'
emit 'a=foobar; echo "${a:0:3}"; echo "${a:3}"'

# quoting + special chars
emit 'echo "a  b   c"'
emit 'echo a  b   c'
emit 'echo "*"; echo "?"'
emit "echo '\$HOME stays literal'"
emit 'echo "exit=$?"'
emit 'x="y z"; for w in $x; do echo "w=$w"; done'
emit 'IFS=,; x="a,b,c"; for w in $x; do echo "$w"; done'

# [[ ]] deeper combos with expansions and arithmetic context
for n in 0 1 7 15; do
	emit "n=$n; [[ \$n -ge 0 && \$n -le 15 ]] && echo \"\$n in range\""
	emit "n=$n; [[ \$(( n * 2 )) -eq $((n*2)) ]] && echo arithok"
done
emit 'v=abc; [[ -n $v && ${#v} -eq 3 && $v == abc ]] && echo triple'
emit 's=""; [[ -z $s && ! -n $s ]] && echo doubly_empty'
emit 'x=5; y=5; [[ $x -eq $y && $x == $y ]] && echo eqboth'
emit 'for i in 1 2 3 4 5; do [[ $((i % 2)) -eq 0 ]] && echo "$i even" || echo "$i odd"; done'

echo "GEN: pass=$pass fail=$fail (failed cases in $WORK/failed.txt)"
echo "wrote $(wc -l < "$OUT") tests to $OUT"

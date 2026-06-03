#!/bin/sh
# String manipulation toolkit using POSIX parameter expansion exclusively (plus
# a few tr/sed touches). Reverse, upper/lower (via tr), trim, split/join, replace
# (manual loop), palindrome, word count, CSV field pick, template fill.
# Exercises: ${#s}, ${s#p}, ${s%p}, ${s##p}, ${s%%p}, ${s:-}, ${s:=}, ${s:+},
# nested expansion, while loops over chars, case, command substitution.
set -u

slen() { echo "${#1}"; }

reverse() {
	s=$1; out=""
	while [ -n "$s" ]; do
		out="$(printf '%s' "$s" | cut -c1)$out"
		s=${s#?}
	done
	echo "$out"
}

trim() {
	s=$1
	while [ "${s# }" != "$s" ]; do s=${s# }; done
	while [ "${s%	}" != "$s" ]; do s=${s%	}; done
	while [ "${s% }" != "$s" ]; do s=${s% }; done
	echo "$s"
}

repeat() { # str n
	s=""; i=0
	while [ "$i" -lt "$2" ]; do s="$s$1"; i=$((i+1)); done
	echo "$s"
}

replace_all() { # haystack needle repl   (manual, no sed)
	hay=$1; nee=$2; rep=$3; out=""
	while [ -n "$hay" ]; do
		case $hay in
			"$nee"*)
				out="$out$rep"
				hay=${hay#"$nee"}
				;;
			*)
				out="$out$(printf '%s' "$hay" | cut -c1)"
				hay=${hay#?}
				;;
		esac
	done
	echo "$out"
}

is_palindrome() {
	s=$1; r=$(reverse "$s")
	[ "$s" = "$r" ] && echo yes || echo no
}

count_words() {
	set -- $1
	echo $#
}

echo "=== lengths ==="
for w in "" a hello "hello world" "tab	here"; do
	printf 'len[%s]=%s\n' "$w" "$(slen "$w")"
done

echo "=== reverse ==="
for w in abc "Hello" "12345" "racecar"; do
	printf 'rev(%s)=%s\n' "$w" "$(reverse "$w")"
done

echo "=== trim ==="
printf '[%s]\n' "$(trim "   spaces   ")"
printf '[%s]\n' "$(trim "	tabs	")"
printf '[%s]\n' "$(trim "  mixed  end ")"

echo "=== repeat ==="
printf '%s\n' "$(repeat = 20)"
printf '%s\n' "$(repeat ab 5)"

echo "=== replace_all ==="
printf '%s\n' "$(replace_all "a.b.c.d" "." "/")"
printf '%s\n' "$(replace_all "aaa" "a" "bb")"
printf '%s\n' "$(replace_all "no match here" "xyz" "Q")"
printf '%s\n' "$(replace_all "foofoofoo" "foo" "bar")"

echo "=== palindrome ==="
for w in racecar hello noon abcba abc; do
	printf '%s: %s\n' "$w" "$(is_palindrome "$w")"
done

echo "=== word count ==="
printf '%s\n' "$(count_words "one two three four")"
printf '%s\n' "$(count_words "   leading and   multiple   spaces  ")"
printf '%s\n' "$(count_words "")"

echo "=== default/alternate parameter expansion ==="
unset opt || true
echo "default: ${opt:-fallback}"
echo "set-default: ${opt:=assigned}"
echo "now opt=$opt"
echo "alt: ${opt:+present}"
unset empty2 || true; empty2=""
echo "empty alt: ${empty2:+should-not-show}x"
echo "empty default: ${empty2:-was-empty}"

echo "=== nested + path manipulation ==="
file="/home/user/docs/report.final.txt"
echo "dir=${file%/*}"
echo "name=${file##*/}"
base=${file##*/}
echo "stem=${base%.*}"
echo "ext=${base##*.}"
echo "allext=${base#*.}"
echo "noext=${base%%.*}"

echo "=== template fill ==="
tmpl="Hello NAME, you have COUNT messages"
out=$(replace_all "$tmpl" "NAME" "Alice")
out=$(replace_all "$out" "COUNT" "7")
echo "$out"

echo "=== upper/lower via tr ==="
echo "HELLO world" | tr 'a-z' 'A-Z'
echo "HELLO world" | tr 'A-Z' 'a-z'

echo "=== char frequency of 'mississippi' ==="
s=mississippi
for ch in i m p s; do
	cnt=0; tmp=$s
	while [ -n "$tmp" ]; do
		c=$(printf '%s' "$tmp" | cut -c1)
		[ "$c" = "$ch" ] && cnt=$((cnt+1))
		tmp=${tmp#?}
	done
	printf '%s:%d ' "$ch" "$cnt"
done
echo
echo "done"

#!/bin/sh
# A character-by-character tokenizer (state machine) for a tiny expression
# language, plus an INI-style config parser. Exercises: parameter expansion
# (${s%?}, ${s#?}, ${#s}, ${s%"$x"}), case with char classes, nested functions,
# while loops over string chars, command substitution, no external tools for
# the lexer (pure shell).
set -u

# Emit one char at a time from a string using ${s#?} / substring-by-cut.
first_char() { printf '%s' "$1" | cut -c1; }
rest() { printf '%s' "${1#?}"; }

classify() {
	c=$1
	case $c in
		[0-9]) echo DIGIT ;;
		[a-zA-Z_]) echo ALPHA ;;
		' '|'	') echo SPACE ;;
		'+'|'-'|'*'|'/') echo OP ;;
		'('|')') echo PAREN ;;
		'='|'<'|'>') echo CMP ;;
		*) echo OTHER ;;
	esac
}

tokenize() {
	s=$1
	cur=""
	curtype=""
	emit() {
		[ -n "$cur" ] && printf '%s(%s) ' "$curtype" "$cur"
		cur=""; curtype=""
	}
	while [ -n "$s" ]; do
		c=$(first_char "$s")
		s=$(rest "$s")
		t=$(classify "$c")
		case $t in
			DIGIT|ALPHA)
				if [ "$curtype" = "$t" ] || { [ "$curtype" = ALPHA ] && [ "$t" = DIGIT ]; }; then
					cur="$cur$c"
				else
					emit
					cur=$c; curtype=$t
				fi
				;;
			SPACE) emit ;;
			*) emit; printf '%s(%s) ' "$t" "$c" ;;
		esac
	done
	emit
	echo ""
}

echo "=== tokenize expressions ==="
tokenize "x1 + 42 * (y - 3)"
tokenize "foo_bar = baz12 / 7"
tokenize "a>=b<c"
tokenize "12345"

echo "=== token type histogram ==="
toks=$(tokenize "alpha 12 + beta3 * (gamma - 99) = delta")
echo "$toks" | tr ' ' '\n' | sed 's/(.*//' | grep -v '^$' | sort | uniq -c | while read -r n t; do
	printf '%-6s %s\n' "$t" "$n"
done

# INI parser: sections [name], key=value, comments ';' or '#'
parse_ini() {
	section="(root)"
	while IFS= read -r line; do
		# trim leading/trailing spaces via parameter expansion loop
		while [ "${line# }" != "$line" ]; do line=${line# }; done
		while [ "${line% }" != "$line" ]; do line=${line% }; done
		case $line in
			''|';'*|'#'*) continue ;;
			'['*']')
				inner=${line#[}
				inner=${inner%]}
				section=$inner
				printf 'SECTION %s\n' "$section"
				;;
			*=*)
				key=${line%%=*}
				val=${line#*=}
				key=${key% }
				val=${val# }
				printf '  %s.%s = %s\n' "$section" "$key" "$val"
				;;
			*)
				printf '  ??? %s\n' "$line"
				;;
		esac
	done
}

echo "=== INI parse ==="
parse_ini <<'INI'
; global config
timeout = 30
[server]
  host = localhost
  port = 8080
[db]
user=admin
# inline comment line
password = secret123
INI

echo "=== suffix/prefix strip drills ==="
path=/usr/local/bin/program.sh
echo "base=${path##*/}"
echo "dir=${path%/*}"
echo "noext=${path%.sh}"
echo "ext=${path##*.}"
echo "firstdir=${path#/}"
f=archive.tar.gz
echo "stem=${f%%.*}"
echo "lastext=${f##*.}"
echo "drop_gz=${f%.gz}"
echo "len=${#path}"
echo "done"

#!/bin/sh
# A small template engine: {{var}} substitution, {{#if VAR}}..{{/if}} blocks,
# {{#each FILE}}..{{/each}} line loops with $line, and a record renderer.
# Pure POSIX string manipulation (parameter expansion + a little sed/tr).
# Exercises: functions, while-read, case, parameter expansion, command sub,
# nested loops, here-docs.
set -u
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# --- variable store backed by a file: key=value lines ---
store="$work/vars"
: > "$store"
set_var() { printf '%s=%s\n' "$1" "$2" >> "$store"; }
get_var() {
	v=$(grep "^$1=" "$store" 2>/dev/null | tail -1)
	printf '%s' "${v#*=}"
}

set_var title "Monthly Report"
set_var user "Alice"
set_var count 42
set_var status active

# --- substitute {{KEY}} occurrences in a single line ---
render_line() {
	line=$1
	out=""
	while [ -n "$line" ]; do
		case $line in
			*"{{"*)
				pre=${line%%"{{"*}
				rest=${line#*"{{"}
				key=${rest%%"}}"*}
				rest=${rest#*"}}"}
				out="$out$pre$(get_var "$key")"
				line=$rest
				;;
			*)
				out="$out$line"
				line=""
				;;
		esac
	done
	printf '%s\n' "$out"
}

echo "=== variable substitution ==="
render_line "Title: {{title}}"
render_line "Hello {{user}}, you have {{count}} items."
render_line "Status is {{status}} (user={{user}})."
render_line "No placeholders here."
render_line "{{title}}{{user}}{{count}}"

echo "=== conditional blocks ==="
render_template() {
	skip=0
	while IFS= read -r line; do
		case $line in
			'{{#if '*'}}')
				cond=${line#'{{#if '}
				cond=${cond%'}}'}
				val=$(get_var "$cond")
				if [ -n "$val" ] && [ "$val" != 0 ]; then skip=0; else skip=1; fi
				;;
			'{{/if}}') skip=0 ;;
			*)
				[ "$skip" -eq 0 ] && render_line "$line"
				;;
		esac
	done
}

render_template <<'TMPL'
=== welcome {{user}} ===
{{#if status}}
status block: {{status}}
count is {{count}}
{{/if}}
{{#if missing}}
this should be hidden
{{/if}}
always shown: {{title}}
TMPL

echo "=== each loop over a data file ==="
cat > "$work/items" <<'ITEMS'
apple 3
banana 5
cherry 12
date 7
ITEMS

render_each() {
	file=$1
	total=0
	while read -r name qty; do
		printf '  - %-8s x%s\n' "$name" "$qty"
		total=$((total + qty))
	done < "$file"
	printf 'total qty: %d\n' "$total"
}
render_each "$work/items"

echo "=== record renderer (CSV -> formatted) ==="
cat > "$work/people" <<'PPL'
Alice,30,Engineering
Bob,25,Sales
Carol,35,Research
PPL
while IFS=, read -r name age dept; do
	set_var rname "$name"
	set_var rage "$age"
	set_var rdept "$dept"
	render_line "{{rname}} ({{rage}}) works in {{rdept}}"
done < "$work/people"

echo "=== nested: table from template + data ==="
gen_row() {
	set_var c1 "$1"; set_var c2 "$2"
	render_line "| {{c1}} | {{c2}} |"
}
gen_row "Name" "Value"
gen_row "----" "-----"
i=1
while [ "$i" -le 3 ]; do
	gen_row "row$i" "$((i * i))"
	i=$((i+1))
done

echo "=== escape-ish: literal braces that aren't placeholders ==="
render_line "code: if (x) { y; } end"
render_line "single { brace and } here"
echo "done"

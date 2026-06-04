#!/bin/sh
# Larger integrated program: a tiny "todo" processor reading commands from a
# here-doc, writing state to a tmp file, using functions, case, loops, redirs.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
db="$work/todo.db"
: > "$db"

add_item() {
	echo "$1" >> "$db"
}

list_items() {
	n=1
	while read -r line; do
		echo "$n. $line"
		n=$((n + 1))
	done < "$db"
}

count_items() {
	wc -l < "$db"
}

# Process a command stream
while read -r cmd arg; do
	case "$cmd" in
		add)
			add_item "$arg"
			echo "added: $arg"
			;;
		count)
			echo "count: $(count_items)"
			;;
		list)
			echo "list:"
			list_items
			;;
		"")
			;;
		*)
			echo "unknown: $cmd"
			;;
	esac
done <<'EOF'
add buy milk
add walk dog
add write code
count
list
EOF

echo "-- final --"
echo "total items: $(count_items)"
list_items | sort

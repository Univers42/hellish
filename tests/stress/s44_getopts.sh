#!/bin/sh
# getopts option parser wrapped in a function so it can be exercised with fixed
# argument lists (OPTIND is reset before each run).
parse() {
	OPTIND=1
	verbose=0; name="anon"; count=1
	while getopts "vn:c:" opt; do
		case "$opt" in
		v) verbose=1 ;;
		n) name="$OPTARG" ;;
		c) count="$OPTARG" ;;
		*) echo "bad option"; return 1 ;;
		esac
	done
	shift $((OPTIND - 1))
	printf 'verbose=%d name=%s count=%s remaining=%d:' "$verbose" "$name" "$count" "$#"
	for a in "$@"; do printf ' %s' "$a"; done
	printf '\n'
}
parse -v -n alice -c 3 file1 file2
parse -n bob extra
parse file_only
parse -v

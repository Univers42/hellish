#!/bin/sh
# A reusable argument-parser "library": short flags, short-with-arg, long flags,
# --long=value and --long value, combined short flags (-vx), -- terminator,
# positionals, defaults, validation, usage. Driven over several synthetic argv
# sets. Exercises: functions, case with globs, while/shift, parameter expansion,
# string ops, set -- to build argv, command substitution.
set -u

# Parse one argv (passed as "$@"). Sets global result vars via a flat string.
parse_args() {
	opt_verbose=0
	opt_force=0
	opt_output="stdout"
	opt_level=1
	opt_name=""
	positionals=""
	parse_err=""
	while [ $# -gt 0 ]; do
		case $1 in
			--)
				shift
				while [ $# -gt 0 ]; do positionals="$positionals $1"; shift; done
				break
				;;
			--verbose) opt_verbose=$((opt_verbose+1)); shift ;;
			--force) opt_force=1; shift ;;
			--output=*) opt_output=${1#--output=}; shift ;;
			--output)
				if [ $# -lt 2 ]; then parse_err="--output needs arg"; return 2; fi
				opt_output=$2; shift 2
				;;
			--level=*) opt_level=${1#--level=}; shift ;;
			--name=*) opt_name=${1#--name=}; shift ;;
			--*) parse_err="unknown long: $1"; return 3 ;;
			-)
				positionals="$positionals -"; shift ;;
			-*)
				# combined short flags: strip leading '-', process each char
				flags=${1#-}
				shift
				while [ -n "$flags" ]; do
					c=$(printf '%s' "$flags" | cut -c1)
					flags=${flags#?}
					case $c in
						v) opt_verbose=$((opt_verbose+1)) ;;
						f) opt_force=1 ;;
						o)
							# -o takes the rest of this token, or next arg
							if [ -n "$flags" ]; then
								opt_output=$flags; flags=""
							elif [ $# -gt 0 ]; then
								opt_output=$1; shift
							else
								parse_err="-o needs arg"; return 2
							fi
							;;
						*) parse_err="unknown short: -$c"; return 4 ;;
					esac
				done
				;;
			*)
				positionals="$positionals $1"; shift ;;
		esac
	done
	return 0
}

report() {
	printf 'verbose=%d force=%d output=%s level=%s name=[%s] pos=[%s] err=[%s] rc=%d\n' \
		"$opt_verbose" "$opt_force" "$opt_output" "$opt_level" "$opt_name" \
		"$positionals" "$parse_err" "$1"
}

run() {
	label=$1
	shift
	parse_args "$@"
	rc=$?
	printf '%-22s' "$label:"
	report "$rc"
}

echo "=== argument parsing cases ==="
run "simple flags"      -v -f
run "combined short"    -vvf
run "long verbose"      --verbose --verbose
run "long eq output"    --output=/tmp/log
run "long sep output"   --output /var/log
run "short o joined"    -ofile.txt
run "short o sep"       -o out.bin
run "level + name"      --level=5 --name=widget
run "positionals"       a b c
run "dash terminator"   -v -- -x --not-a-flag pos1
run "mixed"             -vf --output=X file1 file2
run "unknown long"      --bogus
run "unknown short"     -z
run "missing arg"       --output
run "lone dash"         -
run "empty"

echo "=== validation pass: build a command line ==="
validate() {
	parse_args "$@"
	[ -n "$parse_err" ] && { echo "REJECT: $parse_err"; return 1; }
	case $opt_level in
		[0-9]) echo "ACCEPT level=$opt_level output=$opt_output" ;;
		*) echo "REJECT: bad level $opt_level" ;;
	esac
}
validate --level=3 --output=run.log a b
validate --level=99
validate -vf -o data.csv input

echo "=== count flags across many invocations ==="
total_v=0
for argset in "-v" "-vv" "-vvv" "--verbose" "-v --verbose"; do
	parse_args $argset
	total_v=$((total_v + opt_verbose))
done
echo "total_verbose=$total_v"
echo "done"

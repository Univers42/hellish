#!/bin/sh
# Template engine driven by ${var//pat/rep}: generate 200 template lines
# with @PLACEHOLDER@ markers, then render each by chained pattern
# substitution. Stresses ${//} and quoted expansion in a loop.
render() {
	line=$1
	line=${line//@NAME@/$2}
	line=${line//@CITY@/$3}
	line=${line//@ID@/$4}
	printf '%s\n' "$line"
}

i=0
count=0
while [ $i -lt 200 ]; do
	tpl="user @NAME@ (id @ID@) lives in @CITY@; contact: @NAME@@@CITY@.example"
	case $i in
		*[02468])
			out=$(render "$tpl" alice wonderland "$i")
			;;
		*)
			out=$(render "$tpl" bob builderton "$i")
			;;
	esac
	count=$((count + ${#out}))
	i=$((i+1))
done
echo "$out"
echo "chars=$count"

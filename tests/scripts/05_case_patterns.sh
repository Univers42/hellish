#!/bin/sh
# case with alternation, glob patterns, and the default branch.
for s in apple apricot banana cherry 42 ""; do
	case "$s" in
		a*)
			echo "$s: starts with a"
			;;
		b* | c*)
			echo "$s: b or c"
			;;
		[0-9]*)
			echo "$s: numeric-ish"
			;;
		"")
			echo "(empty)"
			;;
		*)
			echo "$s: other"
			;;
	esac
done

# case on a glob-style suffix
for f in main.c util.h notes.txt run.sh; do
	case "$f" in
		*.c | *.h) echo "$f -> source" ;;
		*.sh) echo "$f -> script" ;;
		*) echo "$f -> data" ;;
	esac
done

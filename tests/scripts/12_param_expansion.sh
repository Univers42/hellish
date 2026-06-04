#!/bin/sh
# POSIX parameter expansion forms (avoiding bash-only ${x/y/z}).
path=/usr/local/bin/program
echo "basename-ish: ${path##*/}"
echo "dirname-ish:  ${path%/*}"
echo "first-comp:   ${path#/}"
echo "drop-ext:     ${path%program}"

name=hello
echo "len=${#name}"

unset maybe
echo "default:   ${maybe:-fallback}"
echo "still-unset for :- => [${maybe}]"
echo "assign:    ${maybe:=assigned}"
echo "now-set:   [${maybe}]"

set_val=value
echo "plus:      ${set_val:+present}"
echo "plus-unset:[${other:+present}]"

file=archive.tar.gz
echo "greedy-pct:  ${file%%.*}"
echo "lazy-pct:    ${file%.*}"
echo "greedy-hash: ${file##*.}"
echo "lazy-hash:   ${file#*.}"

#!/bin/sh
# XFAIL-UNTIL-IMPLEMENTED: `local` in functions
# `local` is a (widely supported, non-strict-POSIX) builtin being implemented
# separately. With proper `local`, the outer `v` stays "outer". Listed as
# expected-to-fail until `local` lands.
v=outer
f() {
	local v
	v=inner
	echo "inside f: v=$v"
}
f
echo "outside f: v=$v"

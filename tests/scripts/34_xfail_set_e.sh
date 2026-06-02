#!/bin/sh
# XFAIL-UNTIL-IMPLEMENTED: set -e (errexit)
# This exercises `set -e`, which is being implemented separately. Under bash
# --posix the script should abort at the first failing simple command, so
# "after-false" must NOT print. Listed as expected-to-fail until errexit lands.
set -e
echo "before"
false
echo "after-false should not print"

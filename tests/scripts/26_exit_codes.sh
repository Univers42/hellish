#!/bin/sh
# Exit status propagation through &&, ||, ;, pipelines, and explicit exit.
true && echo "after true&&"
false || echo "after false||"
false && echo "should not print"
true || echo "should not print"

echo "rc of true: $(true; echo $?)"
echo "rc of false: $(false; echo $?)"

# pipeline exit status is that of the last command (POSIX)
true | false
echo "true|false rc=$?"
false | true
echo "false|true rc=$?"

# command not found gives 127
nosuchcmd_xyz 2>/dev/null
echo "missing cmd rc=$?"

# explicit exit code via subshell so parent continues deterministically
(exit 42)
echo "explicit rc=$?"

# chained logic with grouping
{ false || true; } && echo "group succeeded"

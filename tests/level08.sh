echo === LEVEL 08: While and Until Loops ===
x=0
while [ $x -lt 5 ]; do echo "while: $x"; x=$((x+1)); done
y=3
until [ $y -eq 0 ]; do echo "until: $y"; y=$((y-1)); done
count=0
while [ $count -lt 3 ]; do echo "count is $count"; count=$((count + 1)); done
echo "final count: $count"
n=10
while [ $n -gt 0 ]; do echo -n "$n "; n=$((n - 2)); done
echo "liftoff"

#!/bin/sh
# Background jobs + wait, deterministically (synchronize with wait, compare exit
# codes, never print pids/timing). Exercises &, wait, wait $!, $!, $?, subshells,
# exit codes from background jobs.
set -u
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

echo "=== single background job + wait ==="
(echo "bg output" > "$work/a"; exit 0) &
wait $!
echo "rc=$? out=[$(cat "$work/a")]"

echo "=== background exit code via wait \$! ==="
(exit 7) &
wait $!
echo "exit7_rc=$?"
(exit 0) &
wait $!
echo "exit0_rc=$?"

echo "=== several bg jobs, wait for all ==="
i=1
while [ "$i" -le 5 ]; do
	(echo "$i" > "$work/job_$i") &
	i=$((i+1))
done
wait
sum=0
for f in "$work"/job_*; do
	v=$(cat "$f")
	sum=$((sum + v))
done
echo "all jobs done, sum=$sum"

echo "=== producer/consumer via a file ==="
(
	j=0
	while [ "$j" -lt 10 ]; do echo "item-$j"; j=$((j+1)); done > "$work/queue"
) &
wait $!
echo "queue lines: $(wc -l < "$work/queue" | tr -d ' ')"

echo "=== background pipeline ==="
(seq 1 20 | grep -c '[0-9]' > "$work/cnt") &
wait $!
echo "pipeline counted: $(cat "$work/cnt")"

echo "=== \$! tracks the most recent background pid (non-empty) ==="
sleep 0 &
last=$!
[ -n "$last" ] && echo "last bg pid is set" || echo "no bg pid"
wait

echo "=== sequential waits preserve each exit code ==="
(exit 1) & p1=$!
(exit 2) & p2=$!
(exit 3) & p3=$!
wait $p1; r1=$?
wait $p2; r2=$?
wait $p3; r3=$?
echo "r1=$r1 r2=$r2 r3=$r3"

echo "=== bg job modifies file, parent reads after wait ==="
echo "start" > "$work/log"
(echo "from-bg" >> "$work/log") &
wait
printf 'log:\n'
cat "$work/log"

echo "=== wait with no args returns 0 when all done ==="
wait
echo "final wait rc=$?"
echo "done"

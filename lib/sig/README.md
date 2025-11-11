# Signal Library Demo - Zombie & Leak Testing

## Purpose
This demo showcases how to use the signal library to manage child processes and demonstrates the consequences of improper signal handling, including:
- **Zombie processes**: children that terminated but were not reaped
- **Resource leaks**: parent-side buffers not freed; child-side allocations never cleaned up

## Protection Modes
- `protect 0`: No automatic reaping. Zombies accumulate.
- `protect 1`: SIGCHLD handler reaps children automatically.
- `protect 2`: SA_NOCLDWAIT kernel-level reaping (no zombies created).

## Leak Mode
- `leak off`: Parent frees buffers on cleanup; children allocate nothing extra.
- `leak on`: Parent **does not** free buffers; children allocate an extra 256KB that is never freed (child-side leak, visible only if you valgrind the child process separately).

## Test Scenarios

### Scenario 1: Parent-side leaks with `leak on` + `crash`
```bash
valgrind --leak-check=full ./bin/sig_demo
> leak on
> spawn 3
> status
> crash
```
**Expected**: Parent shows ~192KB "still reachable" (3 × 64KB parent buffers not freed). Children also have leaks internally (256KB each), but only visible if you run valgrind on a child PID.

### Scenario 2: Zombies with `protect 0` + `crash`
```bash
valgrind --leak-check=full ./bin/sig_demo
> protect 0
> spawn 3
> crash
# In another terminal:
ps aux | grep worker
```
**Expected**: 3 zombie `<defunct>` processes remain until the system init reaps them. Parent leaks are also visible if leak mode was on.

### Scenario 3: Clean exit with proper protection
```bash
valgrind --leak-check=full ./bin/sig_demo
> protect 1
> leak off
> spawn 3
> exit
```
**Expected**: No leaks, no zombies. All resources cleaned.

### Scenario 4: Compare protect modes under leak + crash
```bash
valgrind --leak-check=full ./bin/sig_demo
> protect 0
> leak on
> spawn 3
> crash
```
vs
```bash
valgrind --leak-check=full ./bin/sig_demo
> protect 1
> leak on
> spawn 3
> crash
```
**Expected**: With protect 0, children become zombies; with protect 1, SIGCHLD reaps them immediately but parent leaks remain visible in valgrind.

## Commands
- `spawn [n]`: Create n child processes
- `kill [idx|all]`: Kill specific child or all
- `zombie [idx]`: Send SIGTERM to make a zombie (if auto_reap off)
- `protect 0|1|2`: Set zombie protection mode
- `leak on|off`: Toggle leak simulation
- `status` or `ps`: Show process status
- `crash`: Simulate crash (exit without cleanup)
- `exit`: Clean exit

## Key Observations
- Valgrind on the **parent** shows parent-side leaks (pipe buffers, child metadata).
- Child-side leaks (256KB malloc) are **not visible** in parent's valgrind; you must attach valgrind to a child PID separately.
- Zombies consume PID table entries and remain until reaped.
- `protect 1` or `protect 2` prevents zombies; `protect 0` demonstrates the problem.

## Additional Notes
- To observe child-side leaks, run valgrind directly on the child process ID.
- Ensure proper permissions and environment settings when spawning processes or sending signals.
- Use `kill -9` cautiously, as it forcibly terminates processes without cleanup.
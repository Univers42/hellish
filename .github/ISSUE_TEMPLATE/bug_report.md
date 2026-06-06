---
name: Bug report
about: Something hellish does differently from bash, crashes, or leaks
title: "[bug] "
labels: bug
---

## What happened

<!-- A clear description. If hellish disagrees with `bash --posix`, show both. -->

**Command / script:**

```sh
# the smallest input that reproduces it
```

**hellish output / exit status:**

```
```

**bash --posix output / exit status (the expected behaviour):**

```
```

## Environment

- hellish version: <!-- run `update` or check the banner -->
- Build: <!-- `make` / `make OPT=1` / which SAFE? -->
- OS / arch:

## Extra signal (very helpful)

- [ ] It crashes (segfault / abort / `munmap_chunk`) — paste the message
- [ ] AddressSanitizer / LeakSanitizer report (from a `SAFE=1` build):

```
```

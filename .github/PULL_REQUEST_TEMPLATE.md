<!-- Thanks for contributing to hellish! Please fill this out. A PR must solve
     more than it breaks. See CONTRIBUTING.md. -->

## What & why

<!-- What problem does this solve? What's the root cause? Why this approach? -->

Closes #

## How I verified it

<!-- Be specific: which gates, which tests, which inputs. -->

- [ ] `make re` (SAFE=1 debug) builds clean, no warnings
- [ ] `make re OPT=1` (SAFE=0 optimized) builds clean, no warnings
- [ ] `make test` is green (libc heap)
- [ ] `cd tests && ./verify_alloc.sh` green on **both** SAFE=1 and SAFE=0
- [ ] **Added test(s)** covering this bug/feature (and nearby cases)
- [ ] No leaks — checked under ASan/LSan (SAFE=1) and/or `HELLISH_ALLOC_STATS`
      (SAFE=0)
- [ ] No crashes/segfaults/UB, including on edge cases (empty input, bad quotes,
      huge args, deep nesting)
- [ ] `make norm` clean on touched files (no in-function comments, ≤80 cols,
      no new globals)
- [ ] Documented: code comments (Doxygen for complex fns), commit body, this PR

## Notes / trade-offs

<!-- Anything reviewers should know. If the root cause is in vendor/libft or
     ft_malloc, link the submodule PR. -->

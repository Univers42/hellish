# Contributing to hellish

Thanks for wanting to make `hellish` better. This shell holds itself to a high
bar — it diffs byte-for-byte against `bash --posix`, runs under AddressSanitizer,
and stays 42-norm clean — and contributions are expected to keep it there. The
golden rule:

> **A pull request must solve more than it breaks.** If you can't prove your
> change is a net improvement, it isn't ready yet.

---

## The flow, start to finish

1. **Fork** the repository to your own account.
2. **Branch** from `develop` with a descriptive name (see [naming](#branch-naming)).
3. Make your change. Keep it focused — one concern per PR.
4. **Add tests** that cover the bug or feature (see [tests](#every-fix-ships-with-a-test)).
5. **Run all the gates** (see [the checklist](#the-pre-pr-checklist)) until every
   one is green.
6. **Document** what you did — in the code (comments), in the commit body, and in
   the PR description.
7. Open the **PR against `develop`** only once you're sure it introduces no
   regressions and genuinely fixes/adds something.

You never push to `main` or `develop` directly — those are protected. Everything
lands through review.

---

## Branch naming

Use a `type/short-description` slug, matching the kind of work:

```
feat/process-substitution      fix/heredoc-eof
refactor/expander-split        docs/install-steps
test/glob-edge-cases           perf/cmdsub-fast-path
chore/ci-bump
```

---

## Commit format

Commits follow **Conventional Commits** — the `commit-msg` hook enforces it:

```
type(scope): short imperative description

Optional body explaining the WHAT and especially the WHY. Wrap at ~72 cols.
Reference issues like: Closes #123.
```

- **type**: `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `chore`, `style`.
- **scope**: the module — `lexer`, `parser`, `expander`, `executor`, `builtins`,
  `alloc`, `glob`, `heredoc`, `job-control`, … (optional but encouraged).
- Keep the subject ≤ ~72 chars, imperative mood ("add", not "added").
- Do **not** add `Co-authored-by:` trailers for AI tools.

To install the project hooks locally:

```sh
./vendor/scripts/install-hooks.sh   # commit-msg + pre-push validators
```

---

## Every fix ships with a test

This is non-negotiable. **For every problem you find, add a test that covers it**
— and, where you can, a test that covers the neighbouring cases too, so the same
class of bug can't sneak back through a slightly different door.

- The suite lives in `tests/`. Each category is a plain file of one-command-per-
  line cases; the harness runs each through `hellish -c` *and* `bash --posix` and
  diffs stdout + exit status + any files written.
- Larger programs go in `tests/scripts/*.sh` and `tests/hard/*.sh` (run as whole
  scripts vs bash).
- Add your case to the right category file (or a new one), then:

```sh
make test                    # full suite
cd tests && ./tester <file>  # just your category
```

A green suite on **both** allocator backends is required (see below).

### Behaviour the golden suite cannot express

The golden suite diffs `stdout` + status of `hellish -c`. Anything that only
exists in front of a terminal — readline, the prompt, history recall, job
control's use of the tty — needs a pty, and those live in `tests/*.py`.

**Drop the file in `tests/` and you are done.** `make pty-test` globs the
directory, so a new regression test runs in CI the moment it exists:

```sh
make pty-test                          # every tests/*.py
tests/pty_suite.sh -- history_opts     # just the ones matching a pattern
```

That discovery is deliberate. The runner used to be a hand-written list in
two places (a Makefile target and a CI step), and
`completion_posix_test.py` was in neither — it sat in `tests/` running
nowhere from the day it landed, guarding nothing. A list you have to
remember to update is a list that drifts.

Each file takes the shell path as `argv[1]` and nothing else. Keep it that
way; the uniformity is what makes discovery possible.

---

## The pre-PR checklist

Run all of these from a clean tree. Every one must pass before you open a PR.

- [ ] **Builds everywhere, no warnings.** `-Werror` is on; a warning is an error.
  ```sh
  make re            # SAFE=1 debug
  make re OPT=1      # SAFE=0 optimized — both must build clean
  ```
- [ ] **The suite is green on both heaps.**
  ```sh
  make test                       # libc (SAFE=1)
  cd tests && ./verify_alloc.sh   # builds + diffs SAFE=1 AND SAFE=0 vs bash
  ```
- [ ] **The pty gates are green.** These catch what the golden suite cannot
  see, and they are the ones a user actually notices.
  ```sh
  make pty-test                   # every tests/*.py, discovered
  ```
- [ ] **It still builds and runs off this machine.** A warning your compiler
  does not emit and a libc yours does not have are how issues #9 and #42
  reached users.
  ```sh
  make smoke                      # 40-check portability workout, local build
  docker/test.sh alpine fedora    # ... or in a clean container
  ```
- [ ] **No leaks.** The debug build runs under AddressSanitizer + LeakSanitizer
  — run your new cases and read the report. ASan/LSan are only valid on the
  **libc (`SAFE=1`)** build; for the `ft_malloc` side use its own oracle:
  ```sh
  ASAN_OPTIONS=detect_leaks=1 ./build/bin/hellish your_test.sh   # SAFE=1
  HELLISH_ALLOC_STATS=1 ./build/bin/hellish your_test.sh         # SAFE=0 (OPT)
  ```
- [ ] **No crashes.** No segfaults, no `munmap_chunk`/`free()` aborts, no
  undefined behaviour — not on valid input and not on the stupid edge cases
  (empty input, unterminated quotes, huge args, deep nesting). If ASan or the
  shell aborts, it's a bug.
- [ ] **42 norm is clean.** `make norm` reports no violations on the files you
  touched. In particular: no comments inside function bodies, lines ≤ 80
  columns, and **no new global variables** (see below).
- [ ] **It's documented.** See [documenting your work](#documenting-your-work).

If `make test` shows a *new* failure that you can't explain, stop and
investigate — do not paper over it.

---

## Code style & the norm

`hellish` is a 42 project and follows the **42 Norm** (small functions, ≤ 5 per
file, ≤ 80 columns, the standard header on every file, no `for`/ternaries, etc.).
A couple of project-specific rules on top:

- **Comments are block comments only**, placed *before* a function or at file
  scope — never inside a function body. Write them in the house voice: explain
  the *why*, the trick, the gotcha, not a restatement of the code.
- **For a genuinely complex function, use a Doxygen-style block** right before
  it (still outside the body):
  ```c
  /**
   * @brief One-line summary of what this does and why it's tricky.
   * @param state  The shell — single source of truth.
   * @param node   The AST node being executed.
   * @return The execution result (status + pid).
   *
   * The hard part: <the subtle invariant / ordering / ownership rule>.
   */
  ```
- **No new global variables.** The 42 rule is *one* global, and only for the
  signal number. Today the tree still carries a few legacy globals (the word
  slab, the env index, readline-completion iterators, the dir stack) — these are
  a known cleanup target, not a license to add more. New state belongs in
  `t_shell` (or a struct you thread through), never at file scope. Readline's
  completion callbacks are the one painful exception (their signature has no
  user-data pointer); even there, prefer the smallest possible surface.

---

## Documenting your work

- **In the code:** comment the *why*. Doxygen block for anything subtle.
- **In the commit:** a body that explains the reasoning, the bug's root cause,
  and what the test proves.
- **In the PR:** describe the problem, the fix, how you verified it (which gates,
  which tests), and any trade-offs. Screenshots/asciinema for interactive
  changes are gold.

---

## Working on libft or ft_malloc

`vendor/libft` (the standard library **and** the `ft_malloc` allocator) and
`vendor/scripts` are **git submodules** — separate repositories. A bug whose root
cause lives there must be fixed **in that repo**, with its own tests and its own
PR; then the submodule pointer is bumped here. Don't patch around an allocator or
libft bug from inside the shell — fix it at the source.

---

## When it's ready

Open the PR against `develop`. A good PR:

- builds clean on both `SAFE` backends, with no warnings;
- is green on the full suite (both heaps) and adds tests for the change;
- is leak-clean and crash-free under the sanitizers;
- passes `make norm`;
- is documented in code, commit, and description;
- does **not** regress anything — it solves more than it touches.

Thanks for holding the line. 🐚🔥

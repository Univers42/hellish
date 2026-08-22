# Platforms

Where hellish is known to build and run, how that is proved, and what is
still broken. If you hit a platform problem, this page should either
already name it or be the place you add it.

## Why this page exists

Two bugs reached users because CI ran on exactly one machine:

- **Issue #9** — gcc 11's `-Wstringop-overflow` rejected a `va_list`
  forwarded between functions. `make` failed outright on Ubuntu 22.04, the
  distro WSL installs by default. gcc 13 and gcc 14 compiled it in silence,
  so every CI job stayed green.
- **Issue #42** — reported from CachyOS, an Arch derivative, with a
  screenshot instead of a reproducer.

Neither was hard to fix. Both were hard to *see*. The matrix below is the
fix for that, and the reason it is deliberately wide.

## How it is proved

Two things run, and they answer different questions.

| | what it proves | where |
|---|---|---|
| `docker/smoke.sh` | 40 checks: the shell starts, forks, pipes, redirects, here-docs, globs, does arithmetic, handles signals and job control, and reports the right exit statuses | every platform rung |
| the golden suite | ~3800 cases diffed byte-for-byte against a pinned bash 5.3.9 | x86_64 and arm64 Linux |
| `tests/link_closure_test.py` | every symbol the archives reference is defined — *asking GNU ld the question Apple's linker asks by default* | anywhere, via `make pty-test` |
| `tests/linux_only_apis_test.py` | Linux-only kernel interfaces are named in one file, not copied into four | anywhere, via `make pty-test` |
| `tests/crlf_hygiene_test.py` | nothing executed is stored with CRLF, *and* every such file is covered by a rule | anywhere, via `make pty-test` |

The smoke is not a weaker suite — it is a *different* one. It targets the
class of bug that only appears when the C code meets a different libc,
compiler or kernel personality. The golden suite needs a bash 5.3.9 built
from source to diff against; doing that inside fourteen distro images would
turn the matrix into an hours-long job for very little extra signal, so it
runs where it is cheap and the smoke runs everywhere.

Run either one yourself:

```sh
make smoke                      # the portability workout, against your build
make docker-test                # build + smoke in every distro container
docker/test.sh fedora void      # ... or just these
```

## The matrix

### Linux — supported, gated

Every rung builds from source with `-Wall -Wextra -Werror` and then runs the
smoke. A warning is a build failure.

| rung | libc | compiler | why it is in the list |
|---|---|---|---|
| Ubuntu 24.04 | glibc | gcc, clang | the common case |
| Ubuntu 22.04 | glibc | gcc 11 | the default WSL distro, and the oldest toolchain we claim to support — issue #9 lived here |
| Debian stable | glibc | gcc, clang | |
| Alpine 3.20 | **musl** | gcc, clang | a different libc is where "works on my machine" goes to die |
| Alpine 3.20 | musl | gcc, `SAFE=0` | the custom `ft_malloc` heap, on musl — the least portable thing in the tree |
| Arch | glibc | gcc | the family issue #42 came from; newest gcc |
| Fedora 41 | glibc | gcc, clang | the other big non-Debian desktop |
| Rocky 9 | glibc | gcc | old glibc, old gcc — what servers actually run |
| openSUSE Tumbleweed | glibc | gcc | rolling; also the only image here with no `find` (see below) |
| Void Linux | glibc | gcc | xbps, a fifth package manager |
| Linux arm64 | glibc | gcc, clang | a **native** runner, not emulation: alignment, `char` signedness, register width |

### A note on runner scarcity

macOS and WSL run on **push, schedule and manual dispatch — not on pull
requests**, and the distro matrix is capped at `max-parallel: 6`.

That is not tuning for its own sake. On this workflow's first run, the
`macos-13` job sat **queued indefinitely** waiting for an Apple runner. A
queued job keeps its run open, the open run consumes the account's
concurrency, and later pushes got no runs at all — the portability matrix
starved the CI that actually gates the merge. A matrix that stops other CI
from running is worse than no matrix.

On push and schedule a long queue costs nothing. On a PR it blocks the
thing you are waiting for. So the scarce runners live off the PR path.

### macOS — informational

There is no Docker route to macOS. Apple's kernel is not distributable in a
container and the licence does not permit running it off Apple hardware, so
"macOS as Docker" is not a thing that exists — GitHub's `macos-13`
(x86_64) and `macos-14` (arm64) runners are the honest equivalent, and both
are in the `Platforms` workflow.

The job is `continue-on-error` on purpose. macOS bring-up has been
strictly iterative — **six** defects so far, each one only visible once the
one before it was fixed, and every round trip costs a CI queue. In order:
`st_mtim`, `SIGPWR`/`SIGRTMIN`, `bcopy`, `MB_CUR_MAX`, then a link hole
that had nothing to do with Darwin, then `/proc/self/exe`, then this
header collision. As of 2.7.2 the build succeeds and the smoke reached
**39 ok / 1 failed**, that one being `/proc/self/exe`. All known gaps are
now closed:

1. **readline.** Apple ships libedit under the name `libreadline`. It
   answers `-lreadline` and then does not have most of the GNU API this
   shell uses. The Makefile asks `brew --prefix readline` and adds that
   include/lib pair on Darwin. The build and all 40 smoke checks reached
   readline-dependent code without complaint, so this appears settled.
2. **`/proc/self/exe`.** Process substitution re-exec'd the shell through
   that path, so `<(cmd)` and `>(cmd)` produced nothing on Darwin. It is
   now `self_exe_path()` (`src/platform/posix/self_exe.c`) —
   `/proc/self/exe` on Linux, `_NSGetExecutablePath` on Darwin — and the
   ENOEXEC interpreter fallback plus both halves of the update machinery
   went the same way; all four had copied the same Linux-only assumption.
   `/dev/fd/N` for the resulting path was always portable; only the
   self-exec path was not.

   `tests/linux_only_apis_test.py` keeps it that way. It does not test
   process substitution — the golden suite and the smoke already do, on
   Linux, where it always worked. It asserts that `/proc/self/exe` is
   named in exactly one file. That is the property that actually failed:
   the knowledge was in four places, so porting meant finding all four.

3. **`<mach-o/dyld.h>` cannot be included in this tree.** It declares
   `enum DYLD_BOOL { FALSE, TRUE };`, and libft's `ft_stddef.h` already has
   `enum e_bool { FALSE, TRUE }`. Two enums cannot define the same
   enumerator names in one translation unit, so including both is a hard
   error in either order — and it is *invisible* on Linux, where no
   toolchain ships that header at all. `_NSGetExecutablePath` is declared
   by hand instead; one stable ABI symbol is cheaper to declare than an
   enum namespace is to negotiate. Gated in the same test file.

Do not flip `continue-on-error` off until a full run is green on a real
runner. A red required check that everyone learns to ignore is worse than
an informational one.

`-D_XOPEN_SOURCE=700` is also wrong on Darwin: it pins the POSIX.1-2008
surface on glibc and musl, but on Apple's libc it *hides* the BSD extensions
the system headers themselves need. The Makefile uses `-D_DARWIN_C_SOURCE`
there instead.

### WSL — informational

WSL is not "Linux in Docker", and running the Linux containers is not
coverage for it. What differs is the Windows bridge: `drvfs` on `/mnt/c`
has different permission and case semantics, Windows executables appear on
`PATH` and are exec'd through interop, and the default install is Ubuntu
22.04 with gcc 11. Building *inside* WSL is the only way to see any of it,
so the workflow does that on a `windows-latest` runner.

Informational because the runner-side WSL setup is the flakiest step in the
file, not because the platform does not matter.

**Do not build on `/mnt/`.** `$GITHUB_WORKSPACE` is on `D:`, which WSL
reaches through drvfs — a bridge out of the VM to the Windows filesystem.
Every `open`, `stat` and `write` is a round trip, and a build is ~970
compiler invocations doing thousands of them. The first run of this job was
still compiling when the 60-minute timeout killed it, and because the
cancellation lands on the `cmd.exe` wrapper the log ends in
`Terminate batch job (Y/N)?` with nothing useful above it. The job now
copies the tree onto the distro's own ext4 first (one `tar | tar` pass) and
builds there.

What that gives up is drvfs coverage of the *build*, which was never the
point; what it keeps is a separate step that runs the shell against a drvfs
directory — glob it, read from it, `pwd` in it. Deliberately not the
permission checks: `chmod 000` on drvfs without metadata support is a no-op,
so a red there would be Windows telling the truth rather than hellish
getting it wrong.

## Things the matrix has already caught

- **A function declared, called, and never defined.**
  `get_original_tty_job_signals()` was in libft's `trap.h` and called by
  `initialize_traps()`, and no translation unit defined it. Every Linux job
  stayed green for months, because GNU ld lets a *shared library* keep
  undefined symbols and hope the loader finds them later; Apple's linker
  does not, so `libft.so` on arm64 macOS stopped at
  `Undefined symbols for architecture arm64`. This was never a macOS bug —
  it was a library shipping a contract it could not honour, and a runtime
  crash waiting for the first caller. Fixed by defining it (bash's
  semantics: fetch SIGTSTP/SIGTTIN/SIGTTOU once, non-destructively, and
  record SIG_DFL when non-interactive), and pinned by
  `tests/link_closure_test.py`, which reproduces the exact failure on Linux
  in about a second with `-Wl,--no-undefined -Wl,--whole-archive`.
- **`MB_CUR_MAX` is `size_t` on glibc and `int` on Darwin.**
  `mascot_anim.c` tested `mbrtowc`'s result with `n > MB_CUR_MAX` — correct
  on Linux, a `-Werror=sign-compare` build failure on macOS. The rest of the
  tree already spelled the error returns out as `(size_t) - 1` /
  `(size_t) - 2`; now this does too. No behaviour change on any platform,
  which is the point: the two spellings are the same test, and only one of
  them compiles everywhere.
- **`__attribute__((weak))` on a declaration means something else on Mach-O.**
  `alloc_stats.c` probed for ft_malloc's leak oracle with a weak undefined
  reference and a `-Wl,-u` to drag the archive member in, so the file needed
  no `-D`. On ELF an undefined weak symbol is legal and resolves to NULL; on
  Mach-O the same attribute on a *declaration* is a weak **definition**, so
  Apple's linker demanded a body and the SAFE=1 arm64 build stopped at
  `Undefined symbols: _malloc_live_bytes`. Decided at compile time now
  (`-DHAVE_ALLOC_ORACLE`, from the Makefile, which already knows the heap);
  the `-Wl,-u` went with the weak ref, because a *strong* reference pulls an
  archive member by itself. Pinned by `link_closure_test.py`, which fails on
  any weak reference the object tree does not itself define.
- **Windows checkouts rewrite LF to CRLF, and bash cannot read that.**
  `core.autocrlf=true` on the Windows runner turned `set -u` into `set -u\r`
  and `expect() {` into `expect() {\r`, so the WSL rung died inside
  `docker/smoke.sh` with `set: - : invalid option` and a syntax error on a
  brace. `.gitattributes` now pins every executed file type to `eol=lf`, and
  `tests/crlf_hygiene_test.py` gates both halves: nothing executed is stored
  with CRLF, *and* every such file is covered by a rule — the second is what
  catches the next file someone adds, since the first passes on a repo with
  no `.gitattributes` at all.
- **openSUSE Tumbleweed ships no `find`.** The Makefile discovers its
  sources with `$(shell find src ...)`, so the source list came back empty,
  make built nothing, and the link stopped at `cc: fatal error: no input
  files` — an error pointing nowhere near the cause. Fixed twice: the image
  installs `findutils`, and the Makefile now refuses to run with an empty
  `SRCS` and says why.

## Adding a rung

1. Add a service to `docker-compose.yml` (copy an existing one; only `BASE`,
   and optionally `CC` / `BUILD_FLAGS`, change).
2. If it needs a package manager `docker/Dockerfile` does not know, add a
   branch to the `RUN` block — it already covers apk, apt, pacman, dnf,
   zypper and xbps.
3. Add the rung to the `distros` matrix in
   `.github/workflows/platforms.yml`.
4. Add it to the default list in `docker/test.sh` and to the table above.

If the new platform fails, land the rung as informational with the failure
written down here, rather than leaving it out. An unrun platform is a
platform users find bugs on.

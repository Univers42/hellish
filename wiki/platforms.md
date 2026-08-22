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

### macOS — informational

There is no Docker route to macOS. Apple's kernel is not distributable in a
container and the licence does not permit running it off Apple hardware, so
"macOS as Docker" is not a thing that exists — GitHub's `macos-13`
(x86_64) and `macos-14` (arm64) runners are the honest equivalent, and both
are in the `Platforms` workflow.

The job is `continue-on-error` on purpose. The shell is written to POSIX but
has never been built on Darwin, and two gaps are already known:

1. **readline.** Apple ships libedit under the name `libreadline`. It
   answers `-lreadline` and then does not have most of the GNU API this
   shell uses. The Makefile now asks `brew --prefix readline` and adds that
   include/lib pair on Darwin — untested on a real Mac.
2. **`/proc/self/exe`.** Process substitution re-execs the shell through
   that path (`incs/sys.h`), and it does not exist on Darwin. `<(cmd)` and
   `>(cmd)` will not work there until it is replaced with a runtime lookup
   (`_NSGetExecutablePath` on Darwin, `/proc/curproc/file` on FreeBSD).
   Note `/dev/fd/N` is already used for the resulting path, which *is*
   portable — only the self-exec path is not.

`-D_XOPEN_SOURCE=700` is also wrong on Darwin: it pins the POSIX.1-2008
surface on glibc and musl, but on Apple's libc it *hides* the BSD extensions
the system headers themselves need. The Makefile uses `-D_DARWIN_C_SOURCE`
there instead.

Do not flip `continue-on-error` off until the job is green. A red required
check that everyone learns to ignore is worse than an informational one.

### WSL — informational

WSL is not "Linux in Docker", and running the Linux containers is not
coverage for it. What differs is the Windows bridge: `drvfs` on `/mnt/c`
has different permission and case semantics, Windows executables appear on
`PATH` and are exec'd through interop, and the default install is Ubuntu
22.04 with gcc 11. Building *inside* WSL is the only way to see any of it,
so the workflow does that on a `windows-latest` runner.

Informational because the runner-side WSL setup is the flakiest step in the
file, not because the platform does not matter.

## Things the matrix has already caught

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

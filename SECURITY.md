# Security Policy

`hellish` is an educational shell, but a shell is still a program that runs
arbitrary commands, parses untrusted input, and manages file descriptors,
signals, and memory — so we take memory-safety and execution bugs seriously.

## Supported versions

Security fixes land on the latest release line. Older tags are not patched.

| Version | Supported |
|---|---|
| 2.3.x   | ✅ |
| < 2.3   | ❌ (please upgrade) |

## Reporting a vulnerability

**Please do not open a public issue for a security problem.** Public issues are
visible to everyone before a fix exists.

Instead, use **GitHub's private vulnerability reporting**:

1. Go to the repository's **Security** tab → **Report a vulnerability**
   (Security Advisories), or visit
   `https://github.com/Univers42/hellish/security/advisories/new`.
2. Describe the issue, the impact, and the steps (or a minimal script) to
   reproduce it. A crashing input, an ASan trace, or a leak report is ideal.

What counts as a security issue here: memory corruption (buffer overflow,
use-after-free, double-free, cross-heap free), crashes on crafted input,
privilege or environment leaks, command-injection-style parsing bugs, or
anything that lets input do something the user did not intend.

## What to expect

- We aim to acknowledge a report within a few days.
- If confirmed, we'll work on a fix, add a regression test that reproduces it,
  and credit you in the release notes unless you'd rather stay anonymous.
- If the root cause is in a submodule (`vendor/libft` or the `ft_malloc`
  allocator), the fix is made in that repository and the submodule pointer is
  bumped here.

## Hardening you can do

- For real heap/leak coverage, run the **`SAFE=1`** (libc) build under
  AddressSanitizer + LeakSanitizer — that is where the sanitizers are valid.
- The custom allocator (`SAFE=0`) is fast but less battle-tested; for anything
  trust-sensitive, prefer the libc build (`make my_shell` already installs
  `OPT=1 SAFE=1` for exactly this reason).

Thank you for helping keep `hellish` safe.

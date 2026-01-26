# Globbing Module – Conceptual Documentation

This document explains **what the globbing module does and how it is designed**,
without going into line‑by‑line code. The goal is to understand the **idea**
behind the implementation and how the rest of the shell uses it.

At a high level, globbing is what makes this work:

```sh
ls *.c src/[a-z]*.h
```

Patterns like `*.c` or `src/[a-z]*.h` are **expanded into concrete filenames**
before the command runs. This module is responsible for that expansion.

Think of it as a small, self‑contained engine with this contract:

> **Given a pattern string, return the list of filenames that match it, in
>  shell‑like order.**

It is deliberately designed to look and feel like an internal **API** that the
expander can call.

---

## 1. Where Globbing Fits in the Shell

The overall pipeline is:

1. User types a command line.
2. Lexer and parser build an AST (words, redirections, etc.).
3. **Expander** runs environment expansion, command substitution, etc.
4. As part of word expansion, when a word contains `*`, `?`, `[ ... ]` and is
   not quoted, the expander calls the **glob module** to see if it should be
   replaced by matching filenames.
5. The executor then runs the final argv vector produced by the expander.

The glob module **does not know about commands or ASTs directly**. It works on
simple data:

- A string pattern (or, more precisely, a “glob pattern” structure),
- The filesystem (directories and filenames),
- And returns a list of strings.

This separation keeps globbing easy to test in isolation and reusable
wherever pattern expansion is needed.

---

## 2. What the Globbing API Conceptually Provides

From the rest of the shell’s point of view, this module offers:

- A way to **tokenize** a pattern into a sequence of glob tokens:
  - literal segments
  - `*` (match any string)
  - `?` (match a single character)
  - bracket expressions `[ ... ]` (character classes, ranges, POSIX classes)
  - path separators `/` (handled specially for multi‑component patterns)

- A way to **match** those tokens against candidate names.

- A way to **expand a word** (AST node) into:
  - all matching paths, or
  - the original literal word if nothing matched.

- A way to **traverse directories** to support patterns that span multiple
  path segments, like `src/*/*.c`.

- A **sorting routine** (`glob_sort`) that orders the results in a way that
  mimics what users expect from modern shells (case‑insensitive primary order,
  ignoring some non‑alphanumeric differences).

The expander simply calls a function like “expand this word via globbing” and
gets back a vector of `char *`. Everything else is internal to the glob module.

---

## 3. Design Overview – How a Pattern Becomes Filenames

Conceptually, the globbing process is:

1. **Tokenize** the pattern string into a small internal language.
2. **Match** those tokens against directory entries.
3. **Recurse into subdirectories** when the pattern spans multiple components.
4. **Collect and sort** the successful matches.

### 3.1 Tokenization – turning `"*.c"` into tokens

Rather than matching directly on raw strings, the module first converts a
pattern like:

```text
src/[a-z]*.c
```

into a sequence of `t_glob` tokens, such as (conceptually):

- `G_LITERAL("src")`
- `G_SLASH` (a path separator)
- `G_BRACKET` representing `[a-z]`
- `G_ASTERISK` representing `*`
- `G_LITERAL(".c")`

Bracket expressions are parsed into a **character set**:

- `[abc]` → `{ 'a', 'b', 'c' }`
- `[a-z]` → all letters from `a` to `z`
- `[!0-9]` or `[^0-9]` → everything **except** digits
- `[:alpha:]` style POSIX classes are recognized and expanded into prebuilt
  character groups.

Quoted words and certain token types in the AST tell the tokenizer whether glob
characters should be treated as **special** or **literal**. This is how the
expander can respect shell quoting rules while delegating the low‑level work to
the glob module.

### 3.2 Matching – interpreting the tokens

Once we have a sequence of tokens, matching a filename is a recursive
string‑pattern problem:

- **Literal** segments must match exactly.
- `?` must match **exactly one character**, and cannot match a leading `.` when
  used at the start of a filename (this mirrors standard shell behavior: you
  do not accidentally match hidden files unless you say so explicitly).
- `*` must match **zero or more characters** and is the main source of
  backtracking. The logic tries different splits of the input name until a
  consistent match is found or exhausted.
- **Bracket expressions** succeed if the current character is in the expanded
  character set (or out of it, for negated sets).

Matching is also aware of **component boundaries** (`/` tokens): glob magic does
not cross a slash unless the pattern explicitly includes one. This is how
`*.c` only applies to files in the current directory, while `*/foo.c` walks
into subdirectories.

### 3.3 Directory traversal – multi‑component patterns

Patterns like `src/*/*.c` imply a traversal of the filesystem:

1. The first segment `src` identifies a starting directory.
2. The next segment `*` is matched against the entries inside `src/`.
   - For every directory entry that matches, we build a new path `src/entry/`.
3. The final segment `*.c` is matched inside each of those directories.

The glob module encapsulates this traversal:

- It opens directories with `opendir`/`readdir`.
- It builds candidate paths incrementally (e.g. `"src/" + name + "/"`).
- It stops early if a signal requests unwind, so the shell stays responsive.

The caller just gets a **flat list of matching paths**; it does not have to
know how many levels of directories were traversed or how backtracking was
handled.

### 3.4 Sorting – predictable, human‑friendly order

After all matches are collected, they are sorted with a specialized comparator
(`ft_strcoll`) instead of a naive `strcmp`.

The sorting is meant to approximate what users see in common shells with
UTF‑8 locales:

- Case differences are largely ignored for primary ordering (e.g. `foo.c`
  and `FOO.c` group together).
- Non‑alphanumeric characters have lower impact on ordering.
- If all else is equal, a plain lexical `strcmp` is used as a tie‑breaker.

This is implemented purely in terms of ASCII/UTF‑8 rules and does not rely on
external C library locale state, which keeps behavior deterministic and
self‑contained.

---

## 4. Interaction with the Expander and the Rest of the Shell

The glob module is intentionally **dumb** about shell syntax. It only deals with
patterns and filenames. The **expander** decides *when* globbing should happen.

A typical interaction looks like this:

1. The expander has an AST “word” node that has already gone through:
   - tilde expansion,
   - variable and substitution expansion,
   - word splitting (based on `IFS`).

2. If the resulting word contains glob characters and is allowed to be
   expanded (i.e. not in single quotes, not a context where globbing is
   disabled), the expander calls:

   - “expand this word via globbing”, which internally:
     - converts the word into a glob pattern structure,
     - traverses directories and matches filenames,
     - returns the resulting list.

3. If no matches are found, the expander keeps the **original literal** word.
   This is standard shell behavior: `echo '*.c'` prints `*.c` if nothing
   matches.

4. For redirect targets, the expander is stricter: if globbing produces zero or
   more than one filename, it is considered **ambiguous** and results in an
   error like `ambiguous redirect`.

Because the interface is purely in terms of `char *` and simple vectors, the
executor does not need to know that globbing ever happened. It just sees
filenames.

---

## 5. Robustness and Safety Considerations

Even though globbing is a small piece of the shell, it still has some important
robustness goals:

- **No arbitrary recursion into the AST or parser**
  - The module only works with pattern strings and `t_glob` arrays. It does not
    touch environment structures or complex shell state directly.

- **Bounded intermediate storage**
  - Bracket expansion uses fixed‑size buffers for character sets. The code is
    careful not to overflow these buffers; invalid or too‑large ranges degrade
    gracefully instead of corrupting memory.

- **Graceful handling of invalid patterns**
  - Broken bracket expressions (`[a-]`, `[`, `[![:alpha]`, etc.) are typically
    treated as literals or cause the tokenizer to fall back to simpler rules,
    rather than crashing.

- **Signal awareness through the shell state**
  - Directory iteration loops check a global “should unwind” flag so they can
    abort when the user interrupts the shell.

- **Clear memory ownership**
  - Character sets, token arrays, and result vectors have obvious owners and
    dedicated cleanup functions. Callers either **take ownership** of returned
    strings or rely on utility functions to free them.

All of this minimizes the risk that a malformed pattern or a huge directory will
bring down the shell.

---

## 6. Why the Globbing Module Looks Like an API

Just like the expander, globbing is intentionally highly modular and function‑oriented.
This may look “over‑engineered” at first, but it gives several advantages:

1. **Small, testable units**
   - Each step (tokenizing, parsing brackets, matching patterns, traversing
     directories, sorting) is separate. You can understand and test them
     independently.

2. **Reusability in different contexts**
   - The same core operations can be used for:
     - normal argument expansion,
     - redirection targets,
     - potentially other features (like `case` patterns) if desired.

3. **Clear contracts between modules**
   - The expander knows only that “if I pass a word into globbing, I get back a
     list of filenames”. It does not care how `[[:alpha:]]` or `a-z` is parsed.

4. **Future extensibility**
   - Supporting more POSIX classes, locale‑sensitive behavior, or custom sort
     orders can be done **inside** this module without any changes to the
     expander or executor.

In short, the glob module is a **pattern‑matching service**: it takes string
patterns and returns filenames, hiding the complexity of range parsing,
character classes, recursion over directories, and sorting. Keeping it modular
and API‑like makes the rest of the shell simpler and more predictable.
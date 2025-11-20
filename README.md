# Hellish — a POSIX-like shell (sh42)

<p align="center">
  <!-- Badges as button-like links -->
  <a href="https://github.com/Univers42/sh42/actions"><img alt="build" src="https://img.shields.io/badge/build-manual-lightgrey?style=for-the-badge"/></a>
  <a href="LICENSE"><img alt="license" src="https://img.shields.io/badge/license-MIT-blue?style=for-the-badge"/></a>
  <a href="https://github.com/Univers42/sh42/stargazers"><img alt="stars" src="https://img.shields.io/github/stars/Univers42/sh42?style=social"/></a>
  <a href="https://github.com/Univers42/sh42/issues"><img alt="issues" src="https://img.shields.io/github/issues/Univers42/sh42?style=for-the-badge&color=informational"/></a>
</p>

<p align="center">
  <strong>A compact, educational POSIX-like shell written in C — built as a 42 School project.</strong>
</p>

---

Quick links

- Manual (man page): <code>docs/manual/man.1</code>
- Build: <code>make</code>
- Interactive shell: <code>./hellish</code>

Why "Hellish"?

- Implementing a shell touches many tricky corners of UNIX — parsing, expansions, job control and process handling — hence the name. It is intentionally educational and adheres to 42 School norms.

Highlights / Features

- 🧩 Lexer & Parser → AST generation
- 🔍 Expander → variables, tilde, quotes, globbing
- 🔁 Pipelines & redirections
- ⚙️ Builtins with internal exit status semantics
- 🧠 Custom memory allocator (libmemalloc)
- 🔧 Limited job control (background/foreground), limited by curriculum constraints

Quick start

```sh
# Clone (keep submodules)
git clone --recursive git@github.com:Univers42/sh42.git

# Build
make

# Run in interactive mode
./hellish

# See options
./hellish --help

# Read built-in man
groff -Tutf8 -man docs/manual/man.1 | less -R
```

Getting help / demo

- Report issues: https://github.com/Univers42/sh42/issues
- View history & authors: use <code>git log --pretty=format:"%an - %s"</code>
- Want a quick demo GIF? Add one to <code>docs/</code> and link it here for visual appeal.

Repository structure (kept as-is)

<details>
<summary>Click to expand the full project tree (108 directories)</summary>

```bash
.
├── docs
│   ├── manual
│   └── sys
├── incs
├── lib
│   ├── glob
│   │   └── glob_vector
│   ├── libarena
│   │   └── obj
│   ├── libft
│   │   ├── bin
│   │   ├── build
│   │   ├── classes
│   │   │   ├── program_bak
│   │   │   ├── render
│   │   │   │   ├── camera
│   │   │   │   ├── event
│   │   │   │   ├── interface
│   │   │   │   ├── map
│   │   │   │   │   └── test_maps
│   │   │   │   ├── object
│   │   │   │   ├── point
│   │   │   │   └── window
│   │   │   │       └── test_functions
│   │   │   └── trace
│   │   │       └── error
│   │   ├── ctype
│   │   ├── data_structures
│   │   │   ├── array
│   │   │   ├── circular_linked_list
│   │   │   ├── doubly_linked_list
│   │   │   ├── dyn_string
│   │   │   ├── lists
│   │   │   ├── queue
│   │   │   └── vector
│   │   ├── debug
│   │   ├── include
│   │   ├── math
│   │   │   ├── algebra
│   │   │   └── geometry
│   │   │       ├── draw
│   │   │       ├── matrices
│   │   │       ├── quaternion
│   │   │       └── view
│   │   ├── memory
│   │   ├── minilibx-linux
│   │   │   ├── man
│   │   │   │   ├── man1
│   │   │   │   └── man3
│   │   │   └── test
│   │   ├── render
│   │   ├── sort
│   │   ├── stdio
│   │   │   ├── ft_printf
│   │   │   │   ├── include
│   │   │   │   └── src
│   │   │   └── gnl
│   │   ├── stdlib
│   │   │   ├── arena
│   │   │   ├── ft_malloc
│   │   │   ├── pool
│   │   │   ├── slab
│   │   │   └── strto
│   │   ├── strings
│   │   ├── testing
│   │   ├── time
│   │   ├── toolbox
│   │   └── trace
│   ├── liblex
│   │   └── test
│   ├── libmalloc
│   │   ├── bin
│   │   ├── doc
│   │   ├── include
│   │   ├── ref
│   │   │   └── malloc
│   │   │       └── done
│   │   ├── src
│   │   │   ├── core
│   │   │   ├── debug
│   │   │   ├── helpers
│   │   │   └── utils
│   │   └── tests
│   │       └── main
│   ├── libreadline
│   ├── libregexp
│   ├── libvar
│   ├── sh
│   ├── sig
│   │   ├── bin
│   │   ├── obj
│   │   │   └── programs
│   │   └── programs
│   ├── termcap
│   ├── tilde
│   └── wctype
├── scripts
│   └── hooks
└── srcs
    ├── executor
    ├── expander
    ├── globbing
    ├── parser
    ├── shell
    └── utils

108 directories
```

</details>

Project components (short)

- LIBMEMALLOC — custom allocator for learning and fragmentation control.
- LEXER — transforms the command line input into tokens.
- ROUTINE.c — the classic REPL: Read / Execute / Print / Loop. Cleans GC each cycle.
- OPTIONS — command-line flags and options handling.
- PARSING — builds AST from tokens.
- STDOUT — shell output and consistent error printing routines.
- TREE — AST execution engine, handles builtins vs process forking and exit statuses.
- Job control — limited by curriculum constraints; background/foreground handling and job states.

Usage examples

- Simple command:
  ./hellish
  ls -la | grep srcs > out.txt
- Builtin:
  echo "Hello world"
  cd ../
  export PATH=/usr/local/bin:$PATH

Contributing

- Open issues with clear reproduction steps.
- Fork, create a feature branch, add tests and submit a PR.
- Follow the project's coding norms (42 School norm) — keep commits atomic and documented.

Resources & further reading

- POSIX shell specification: https://pubs.opengroup.org/onlinepubs/9699919799/
- Readline library: https://tiswww.case.edu/php/chet/readline/readline.html
- glob.h reference: https://docs.oracle.com/cd/E86824_01/html/E54772/glob.h-3head.html
- shlex (python) reference for parsing ideas: https://rico-schmidt.name/pymotw-3/shlex/

Man page
Use:
groff -Tutf8 -man docs/manual/man.1 | less -R

License & authors

- Project: educational 42 School project. Add LICENSE file to declare terms.
- Authors: see repository history / git log.

<!-- end of file -->

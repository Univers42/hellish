> This document describes the project Shell implementation in C language. It covers the purpose, installation instructions, usage guidelines, and additional resources for further learning.

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#                                       HELLISH - A implementation of shell in C

**Hellish** program has this name because it was our first shell to implement with 42 School. The name was chosen because we tried to respect the most possibly the POSIX norm.

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Norm
The code follows the 42 School norm, which enforces specific coding standards and guidelines to ensure code quality and consistency.

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

# Structure
```bashd
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

# LIBMEMALLOC:
    custom memory allocator to manage dynamic memory allocation efficiently. It provides functions for allocating, freeing, and managing memory blocks, optimizing performance and reducing fragmentation.
# LEXER:
    the lexer is responsible for tokenizing the input command line. It reads the input string and breaks it down into meaningful tokens, such as commands, arguments, operators, and special characters.   
# ROUTINE.c: 
    the program shell is a routine of type READ/EXECUTE/PRINT/LOOP or (REPL). Will perform some initializations, then enter a loop where it will read user input, parse, and execute commands.
    Before each end of loop will empty the garbage collector to free memory.

# OPTIONS:
    handles command-line options and flags passed to the shell program.
# PARSING:
    responsible for analyzing the structure of the input command line and building a parse tree or abstract syntax tree (AST) representing the commands and their relationships.
# STDOUT:
    ash uses it's own out routines to handle printing to the terminal, ensuring consistent formatting and error handling.
    There are routines to print errors, debug information, and command outputs to the terminal.


# TREE:
    tree recursively executes a parse tree. The exit status is returned in the singleton structure.
    The tree is traversed in a depth-first manner, executing commands and handling control structures as defined by the parse tree nodes.
    It supports various node types, including commands, pipelines, logical operators, and subshells.
    it saves the result in memory is the command is a builtin; otherwise it forks off a new process to execute the command and waits for its completion, updating the exit status accordingly.

# Job:
    The job control system manages background and foreground jobs, allowing users to run commands asynchronously.
    Job are routines that handle the execution of commands in the background, manage job states, and provide job control features like bringing jobs to the foreground or sending them to the background.
    As the subject of the school doesn't allow some forbidden function this part will be limited by macros and some workaround.

# Should you be here at all ?
This project is part of the curriculum at 42 School, specifically for students enrolled in the cursus. If you are not a student at 42 School or not involved in this cursus, you might not find this project relevant to your studies or interests.

# Now that you know you should be here...
## Installation

to clone the repository:
```sh
$ git clone --recursive git@github.com:Univers42/sh42.git # in a near future will be changed to hellish
$ make
$ ./hellish --help # using this command you'll see all the options we have implemented in our project
# if you desire use the normal shell
$ ./hellish # The interactive mode routine will be run
```
# dependencies
The project relies on several external libraries and tools to function correctly. Ensure you have the following dependencies installed on your system:
- GNU Make
- GCC or any compatible C compiler
- Readline library
- [malloc library customized] using --recursive you'll get it automatically

---
# ! We have our own man
```
groff -Tutf8 -man docs/manual/man.1 | less -R
```

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Ressources

## Recommendation of tools to use:
[excalidraw](https://marketplace.visualstudio.com/items?itemName=pomdtr.excalidraw-editor)

## about workflow:
- [workflow-github-action syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)
- [learn_yml](https://learnxinyminutes.com/yaml/)

## about system:
- [readline](https://tiswww.case.edu/php/chet/readline/readline.html#Searching)
- [glob.h](https://docs.oracle.com/cd/E86824_01/html/E54772/glob.h-3head.html#scrolltoc)
- [arthmetical operation](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap01.html#tag_17_01_02_01)
- [bash GNU](https://www.gnu.org/software/bash/manual/bash.html#Reserved-Words)
- [shlex](https://rico-schmidt.name/pymotw-3/shlex/)
- [new version of shell definition POSIX 2024](https://pubs.opengroup.org/onlinepubs/9799919799/)

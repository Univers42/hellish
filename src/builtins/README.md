# Builtins Module - Shell Built-in Commands Implementation

## Overview

The `builtins` directory implements the core built-in commands for the sh42 shell. This module provides a complete set of POSIX-compliant shell built-ins with advanced features like redirection parsing, sophisticated argument handling, and optimized dispatch mechanisms.

## Architecture

### Core Design Principles

1. **Modular Architecture**: Each builtin is split into logical components (core logic, helpers, utilities)
2. **Hash-based Dispatch**: O(1) command lookup using a hash table for optimal performance
3. **Unified Argument Processing**: Consistent handling of redirections and argument parsing across all builtins
4. **Memory Safety**: Proper cleanup and error handling throughout
5. **POSIX Compliance**: Faithful implementation of POSIX shell built-in behaviors

### File Organization

```
builtins/
├── builtins_private.h          # Internal API definitions and constants
├── hash_builtins_dispatch.c    # Command dispatch system
├── core_builtins.c            # Main builtin implementations (export, exit, echo, env, unset)
├── core_builtins2.c           # Additional builtins (pwd, cd)
├── echo_flags.c               # Echo flag parsing and processing
├── echo_help.c                # Echo escape sequence handling
├── exit_helpers.c             # Exit command argument validation
├── exit.c                     # Exit cleanup and process management
├── export_helpers.c           # Export argument parsing and validation
├── export_helpers2.c          # Export identifier handling and expansion
├── collect_and_print_exported.c  # Export variable display functionality
├── cd_helpers1.c              # CD command core logic
├── cd_helpers2.c              # CD redirection-aware argument parsing
├── try_unset.c                # Environment variable removal
├── utils.c                    # Argument counting with redirection awareness
└── utils2.c                   # Redirection parsing utilities
```

## Dispatch System

### Hash-based Command Lookup

The builtin dispatch uses a hash table for O(1) command resolution:

```c
typedef int (*t_builtin_fn)(t_shell *state, t_vec argv);
```

**Optimization Strategy:**
- Static hash table initialization on first access
- Function pointers stored directly in hash slots
- No string comparisons during command execution
- Lazy initialization reduces startup time

**Supported Commands:**
- `echo` - Text output with escape sequences
- `export` - Environment variable management  
- `cd` - Directory navigation
- `exit` - Shell termination
- `pwd` - Current directory display
- `env` - Environment variable listing
- `unset` - Environment variable removal

## Echo Implementation Deep Dive

### Architecture

The echo builtin is implemented across two files with sophisticated flag and escape sequence processing:

#### Flag Processing (`echo_flags.c`)

**Algorithm:**
1. **Token-by-token parsing**: Each argument starting with `-` is analyzed
2. **Character validation**: Only `n`, `e`, `E` flags are valid
3. **State accumulation**: Flags modify global state (`-n` suppresses newline, `-e` enables escapes)
4. **Early termination**: Invalid flag characters stop processing

**Key Functions:**
- `parse_flags()`: Returns index of first non-flag argument
- `process_flag_token()`: Validates and applies flag characters
- `apply_flag_char()`: Updates state for individual flags

#### Escape Sequence Processing (`echo_help.c`)

**Supported Escapes:**
- Standard: `\n`, `\t`, `\r`, `\b`, `\f`, `\v`, `\a`, `\\`
- ANSI: `\e` (escape character for terminal control)
- Octal: `\0nnn` (3-digit octal values)
- Hex: `\xHH` (2-digit hexadecimal values)
- Control: `\c` (stop processing immediately)

**Parsing Algorithm:**
1. **Character-by-character scanning**
2. **Backslash detection** triggers escape processing
3. **Numeric escape handling** with base detection (8 or 16)
4. **Immediate termination** on `\c` control sequence

### Advanced Features

**Numeric Escape Processing:**
- Automatic base detection (octal vs hexadecimal)
- Proper bounds checking and type conversion
- Support for partial sequences

## CD Implementation

### Redirection-Aware Argument Parsing

The CD builtin features sophisticated argument parsing that correctly handles shell redirections:

```bash
cd >output.txt /new/directory    # Correctly identifies /new/directory as target
cd 2>/dev/null /path             # Handles stderr redirection
```

**Algorithm (`cd_helpers2.c`):**
1. **Redirection Detection**: Uses `is_redir_operator()` to identify redirect tokens
2. **Skip Logic**: Tracks whether next token should be skipped (redirect target)
3. **Real Argument Extraction**: Returns first non-redirection argument

### Directory Change Logic

**Core Algorithm (`cd_helpers1.c`):**
1. **Special Cases**: 
   - No argument → HOME directory
   - `-` argument → OLDPWD with confirmation output
2. **Error Handling**: Comprehensive errno-based error reporting
3. **State Updates**: PWD/OLDPWD environment variable management

**Error Recovery:**
- Graceful handling of missing HOME/OLDPWD
- Detailed error messages with ctx
- Fallback directory tracking when getcwd() fails

## Export Implementation

### Multi-stage Processing Pipeline

The export builtin uses a sophisticated multi-stage processing pipeline:

#### Stage 1: Argument Parsing (`export_helpers.c`)

**Features:**
- Identifier/value separation at `=` character
- Quote stripping with quote-type tracking
- Following-argument consumption for standalone values
- Variable expansion with quote-awareness

#### Stage 2: Validation and Processing (`export_helpers2.c`)

**Identifier Validation:**
- POSIX-compliant variable name checking
- First character: letter or underscore only
- Subsequent characters: letters, digits, underscores

**Value Processing:**
- Single-quote strings: no expansion
- Double-quote strings: full expansion
- Unquoted strings: full expansion

#### Stage 3: Display (`collect_and_print_exported.c`)

**Algorithm:**
1. **Collection**: Extract all exported variables into vector
2. **Sorting**: Alphabetical sort using quicksort
3. **Formatting**: POSIX-compliant `export NAME="VALUE"` format
4. **Memory Management**: Proper cleanup of temporary strings

## Exit Implementation

### Comprehensive Argument Validation

The exit builtin implements bash-compatible argument processing:

**Validation Pipeline:**
1. **Argument Count**: 0-1 arguments allowed
2. **Double-dash Handling**: Optional `--` separator support
3. **Numeric Validation**: Strict integer parsing with overflow detection
4. **Error Reporting**: ctx-aware error messages

**Exit Code Logic:**
- No arguments: Use last command status
- Numeric argument: Modulo 256 for 8-bit exit codes
- Invalid argument: Exit with code 2 (bash compatibility)
- Too many arguments: Exit with code 1

### Process Management

**Cleanup Strategy:**
- PID verification before cleanup (prevent cleanup in child processes)
- History management for interactive sessions
- Complete state deallocation
- Proper signal handling

## Unset Implementation

### Environment Variable Removal

**Algorithm:**
- Linear search through environment array
- In-place removal with element shifting
- Proper memory deallocation for keys and values
- Length adjustment for vector consistency

**Safety Features:**
- Null pointer checking
- Bounds validation
- No-op for non-existent variables

## Utility Functions

### Redirection-Aware Parsing

**Core Innovation**: All builtins support shell redirections transparently

**Parsing Algorithm (`utils2.c`):**
1. **Pattern Recognition**: Detect `[n]<`, `[n]>`, `[n]>>`, `[n]<<` patterns
2. **Lookahead Logic**: Determine if redirection needs following argument
3. **Skip Management**: Track tokens to ignore during argument processing

**Supported Patterns:**
- `>file`, `>>file`, `<file`
- `2>file`, `1>>file` (file descriptor redirections)
- `2>&1`, `1>&2` (descriptor duplication)

### Argument Counting

**Real Argument Detection (`utils.c`):**
- Skips redirection operators and their targets
- Provides accurate argument counts for validation
- Used by CD for "too many arguments" detection

## Optimization Strategies

### Memory Management

1. **String Interning**: Reuse of common strings where possible
2. **Lazy Allocation**: Hash table initialized only when needed
3. **Efficient Cleanup**: Proper deallocation ordering to prevent leaks
4. **Stack Allocation**: Use stack variables for temporary data

### Performance Optimizations

1. **Hash Dispatch**: O(1) command lookup vs O(n) string comparisons
2. **Short-circuit Evaluation**: Early returns in validation functions
3. **Minimal Allocations**: Reuse existing buffers where safe
4. **Efficient Sorting**: Quicksort for export variable display

### Error Handling

1. **Graceful Degradation**: Continue operation when possible
2. **ctx Preservation**: Maintain error ctx for debugging
3. **Resource Cleanup**: Ensure cleanup on all error paths
4. **User-friendly Messages**: Clear, actionable error messages

## POSIX Compliance Notes

### Standards Adherence

- **Echo**: Full POSIX escape sequence support with `-e` flag
- **Export**: Proper identifier validation and display format
- **CD**: Standard directory navigation with OLDPWD support
- **Exit**: Bash-compatible argument processing and error codes
- **PWD**: Simple current directory display
- **Env**: Standard environment variable listing
- **Unset**: Proper variable removal without errors for missing vars

### Extensions and Enhancements

1. **Redirection Support**: All builtins handle shell redirections
2. **Advanced Error Reporting**: Detailed, ctx-aware error messages
3. **Memory Safety**: Comprehensive bounds checking and cleanup
4. **Performance**: Hash-based dispatch and optimized algorithms

## Integration Points

### Shell State Management

All builtins operate on the central `t_shell` state structure:
- **Environment**: Direct manipulation of environment vector
- **Current Directory**: PWD/OLDPWD maintenance
- **Exit Status**: Proper status code propagation
- **Error ctx**: Consistent error reporting with shell ctx

### Vector and Hash Utilities

Leverages shell's core data structures:
- `t_vec`: Dynamic arrays for arguments and environment
- `t_hash`: High-performance hash table for dispatch
- Memory management through shell allocators

This builtin module provides a robust, efficient, and standards-compliant implementation of essential shell commands with advanced features like redirection awareness and optimized dispatch.
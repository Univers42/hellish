# libmemalloc - Stack-based Arena Allocator

A self-contained, norminette-compliant implementation of dash's memalloc.c/h as a reusable library. Provides efficient stack-based memory management for shells and other command-line tools.

## Features

- **Stack-based allocation**: LIFO allocation pattern for parse trees and temporary data
- **Automatic block growth**: Expands transparently when current block exhausted
- **Generic allocation**: Allocate any data type (not just strings)
- **String building**: Character-by-character and bulk string operations
- **Stack marks**: Save/restore arena state for command cleanup
- **Zero dependencies**: Self-contained, no external requirements
- **Norminette-compliant**: Function-based API, no macro functions
- **Drop-in replacement**: Direct equivalent of dash memalloc.c/h

## Building

```bash
cd lib/libmemalloc
make              # Build libmemalloc.a
make test_memalloc    # Build and run tests
make clean        # Remove object files
make fclean       # Remove everything
```

## Integration

### Option 1: Static Library (Recommended)

1. Copy `libmemalloc.a` to your project
2. Include `arena.h` in your code
3. Link with `-lmemalloc`

```c
#include "arena.h"

int main(void) {
    char *p = (char *)st_alloc(256);
    // use p
    return 0;
}
```

### Option 2: Source Integration

Add all `.c` files from libmemalloc to your build and include `arena.h`.

## API Reference

### Core Allocation

```c
void *st_alloc(size_t nbytes);           /* Allocate nbytes on stack */
void  st_unalloc(void *p);               /* Rewind to p (LIFO) */
```

### Stack Marks (Save/Restore State)

```c
void  push_stack_mark(t_stack_mark *mark, size_t len);
void  set_stack_mark(t_stack_mark *mark);
void  pop_stack_mark(t_stack_mark *mark);
```

Example:

```c
t_stack_mark mark;
set_stack_mark(&mark);
// allocate and use data
pop_stack_mark(&mark);  // rewind to mark
```

### String Building

```c
char *start_stack_str(void);              /* Get current string pointer */
char *st_putc(int c, char *p);            /* Put char, auto-grow */
char *st_nputs(const char *s, size_t n, char *p);  /* Put n chars */
char *st_puts(const char *s, char *p);    /* Put string */
char *stack_str_nul(char *p);             /* Terminate string */
```

Example:

```c
char *p = start_stack_str();
p = st_puts("Hello ", p);
p = st_puts("World", p);
p = stack_str_nul(p);
printf("%s\n", stack_block());
```

### String Helpers

```c
char *stack_block(void);          /* Get current string start */
size_t stack_block_size(void);    /* Space left in current block */
char *st_unputc(char *p);         /* Decrement pointer */
int    st_topc(char *p);          /* Get last char */
char  *st_adjust(ptrdiff_t amt, char *p);  /* Adjust pointer */
void   grab_stack_str(char *p);   /* Allocate used space */
void   ungrab_stack_str(void *s); /* Rewind from grabbed string */
```

### Generic Helpers

```c
void  *arena_malloc(size_t nbytes);         /* malloc wrapper */
void  *arena_realloc(void *p, size_t nbytes); /* realloc wrapper */
void   arena_free(void *p);                 /* free wrapper */
size_t arena_align(size_t nbytes);          /* Align to word */
void   out_of_space(void);                  /* Error handler */
char  *str_save(const char *s);             /* Allocate & copy string */
```

### Context

```c
t_garena *arena_ctx(void);  /* Get the arena context (singleton) */
```

## Types

```c
typedef struct s_stack_mark {
    struct s_stack_block *stackp;
    char *stacknxt;
    size_t stacknleft;
} t_stack_mark;

typedef struct s_garena {
    t_stack_block stack_base;
    t_stack_block *stackp;
    char *stack_next;
    size_t stack_nleft;
    char *sstrend;
} t_garena;
```

## Dash Equivalence

| dash memalloc.h    | libmemalloc          |
| ------------------ | -------------------- |
| `stalloc()`        | `st_alloc()`         |
| `stunalloc()`      | `st_unalloc()`       |
| `pushstackmark()`  | `push_stack_mark()`  |
| `setstackmark()`   | `set_stack_mark()`   |
| `popstackmark()`   | `pop_stack_mark()`   |
| `stackblock()`     | `stack_block()`      |
| `stackblocksize()` | `stack_block_size()` |
| `STPUTC(c, p)`     | `st_putc(c, p)`      |
| `growstackstr()`   | `grow_stack_str()`   |
| `growstackto()`    | `grow_stack_to()`    |
| `makestrspace()`   | `make_str_space()`   |
| `stnputs()`        | `st_nputs()`         |
| `stputs()`         | `st_puts()`          |
| `grabstackstr()`   | `grab_stack_str()`   |
| `ungrabstackstr()` | `ungrab_stack_str()` |
| `ckfree()`         | `arena_free()`       |

## Memory Model

- Single static arena context (singleton)
- LIFO allocation within blocks
- Automatic block chaining when capacity exceeded
- Marks allow save/restore of entire state
- Perfect for parser/AST construction

## Performance

- O(1) allocation (pointer bump)
- O(1) deallocation (pointer rewind via marks)
- Minimal fragmentation (LIFO pattern)
- Single malloc per block growth

## Example: Command Parser

```c
#include "arena.h"

typedef struct s_cmd {
    char *name;
    struct s_cmd *next;
} t_cmd;

void parse_commands(const char **inputs) {
    t_stack_mark base;
    set_stack_mark(&base);

    for (int i = 0; inputs[i]; ++i) {
        t_stack_mark cmd_mark;
        set_stack_mark(&cmd_mark);

        t_cmd *cmd = (t_cmd *)st_alloc(sizeof(t_cmd));
        cmd->name = str_save(inputs[i]);
        cmd->next = NULL;

        // Process command
        execute(cmd);

        // Clean up (O(1) with mark)
        pop_stack_mark(&cmd_mark);
    }

    pop_stack_mark(&base);
}
```

## Testing

Run comprehensive tests with:

```bash
make test_memalloc
valgrind ./test_memalloc
```

Tests cover:

- Generic allocation
- String building
- Stack marks
- Unalloc/rewind
- Grab/ungrab
- Block growth
- Error handling
- Parse tree simulation

## Notes

- The static arena context persists for the program lifetime (process-local singleton)
- All allocations are LIFO; arbitrary freeing will cause issues
- Use `st_unalloc()` only to rewind to a recent allocation
- Use stack marks (`set_stack_mark()` / `pop_stack_mark()`) for scoped cleanup
- Thread-unsafe (like original dash memalloc)

## License

Derived from BSD dash (memalloc.c/h).
See original source for license details.

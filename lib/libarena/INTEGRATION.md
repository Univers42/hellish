# Integration Guide: Using libmemalloc in sh42

This guide shows how to integrate libmemalloc into your sh42 shell project.

## Step 1: Build the Library

```bash
cd lib/libmemalloc
make
ls -la libmemalloc.a
```

You should see `libmemalloc.a` (typically ~30-50 KB).

## Step 2: Update Your Main Makefile

In your project's main `Makefile`, add libmemalloc to the build:

```makefile
LIBS := -L./lib/libmemalloc -lmemalloc
LIBFLAGS := -I./incs -I./lib/libmemalloc/arena.h

# In your linking rule:
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(LIBFLAGS) $(OBJS) $(LIBS) -o $(NAME)

# Add a target to build libmemalloc first:
lib:
	$(MAKE) -C lib/libmemalloc
```

## Step 3: Use in Your Code

Include the header:

```c
#include "arena.h"

int main(int argc, char **argv) {
    // Parse trees
    t_cmd *cmd = (t_cmd *)st_alloc(sizeof(t_cmd));

    // Strings
    char *p = start_stack_str();
    p = st_puts("command", p);
    p = stack_str_nul(p);

    // Cleanup with marks
    t_stack_mark mark;
    set_stack_mark(&mark);
    // ... allocate and use ...
    pop_stack_mark(&mark);  // O(1) cleanup

    return 0;
}
```

## Step 4: Replace dash memalloc Usage

If you have code using dash's memalloc macros, replace them:

### Before (dash style):

```c
#include "memalloc.h"

char *p = STARTSTACKSTR(p);
STPUTC('a', p);
p = CHECKSTRSPACE(n, p);
grabstackstr(p);
```

### After (libmemalloc):

```c
#include "arena.h"

char *p = start_stack_str();
p = st_putc('a', p);
p = make_str_space(n, p);
grab_stack_str(p);
```

## Step 5: Handle intoff/inton (Optional)

If your code uses `intoff()` / `inton()` for signal safety, libmemalloc has them in `error.h`:

```c
#include "error.h"

intoff();  // Critical section start
// allocate
inton();   // Critical section end
```

These are already called in `st_alloc()` when growing blocks.

## Example: Integrating into Parser

### Original (with global variables):

```c
// memalloc.h global declarations
extern char *stacknxt;
extern size_t stacknleft;

// In parser.c
char *p = stackblock();
```

### Refactored (with libmemalloc):

```c
// In parser.c
#include "arena.h"

char *p = stack_block();  // Same semantics
```

## Common Patterns

### Building a String from Tokens

```c
#include "arena.h"

char *build_command(const char **tokens) {
    char *p = start_stack_str();

    for (int i = 0; tokens[i]; ++i) {
        if (i > 0)
            p = st_putc(' ', p);
        p = st_puts(tokens[i], p);
    }

    p = stack_str_nul(p);
    return stack_block();  // Pointer valid for lifetime of mark
}
```

### Allocating Parse Tree

```c
typedef struct s_node {
    int type;
    char *str;
    struct s_node *next;
} t_node;

t_node *parse_expr(void) {
    t_node *node = (t_node *)st_alloc(sizeof(t_node));
    node->type = EXPR_TYPE;
    node->str = str_save(lexer_current_token());
    node->next = NULL;
    return node;
}
```

### Scoped Cleanup

```c
void execute_pipeline(void) {
    t_stack_mark mark;
    set_stack_mark(&mark);  // Save state

    // Parse and execute pipeline
    t_cmd *cmds = parse_pipeline();
    run_cmds(cmds);

    // One pop cleans up everything
    pop_stack_mark(&mark);
}
```

### Multi-level Scopes

```c
void main_loop(void) {
    t_stack_mark loop_mark;
    set_stack_mark(&loop_mark);

    while (read_line()) {
        t_stack_mark line_mark;
        set_stack_mark(&line_mark);

        t_cmd *cmd = parse_line();
        execute(cmd);

        pop_stack_mark(&line_mark);  // Clean line allocations
    }

    pop_stack_mark(&loop_mark);
}
```

## Troubleshooting

### Linking Error: "undefined reference to `st_alloc`"

- Ensure you added `-L./lib/libmemalloc -lmemalloc` to linker flags
- Check that `libmemalloc.a` was built: `ls lib/libmemalloc/libmemalloc.a`

### Redefinition Error

- Don't include both `arena.h` and your old `memalloc.h`
- Replace all memalloc.h includes with `arena.h`

### Rewind Goes Too Far

- Always use `set_stack_mark()` before allocating
- Never call `st_unalloc()` with arbitrary pointers
- Use `pop_stack_mark()` for predictable cleanup

### Valgrind Reports "still reachable"

This is normal and expected. The static arena context persists for the program lifetime:

```
still reachable: 512 bytes in 1 blocks
```

This is NOT a leak; it's the arena metadata that will be freed on process exit.

## Performance Tuning

The arena block size is currently 504 bytes (ARENA_MINSIZE). To adjust:

Edit `arena.h`:

```c
#define ARENA_MINSIZE 1024  /* Larger for fewer block allocations */
```

Larger blocks = fewer malloc calls but more memory overhead.
Smaller blocks = less memory but more allocations.

## Next Steps

1. Build and test: `make test_memalloc`
2. Integrate into your project
3. Replace old memalloc usage
4. Test with valgrind to verify no leaks
5. Profile to tune `ARENA_MINSIZE` if needed

## Questions?

Refer to the comprehensive test suite in `main.c` for usage examples.

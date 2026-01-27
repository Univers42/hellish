# Environment Module Documentation

## 1. Concept: Environment in a Shell

In POSIX shells, the **environment** is a mapping of `KEY=VALUE` strings that is:

- Inherited from the parent process on startup (`envp`),
- Modified by builtins like `export`/`unset`/`cd`,
- Passed to child processes when executing external commands,
- Read by the shell itself for behavior (e.g. `HOME`, `PATH`, `PWD`, `SHLVL`).

This module (`src/environment/`) provides:

- A **structured representation** of the environment (`t_env`, `t_vec_env`),
- Conversion between `char **envp` and that internal representation,
- Environment **lookup and mutation** primitives (`env_get`, `env_set`, `env_create`),
- **Expansion** logic for `$VAR`, `$?`, `$$`, etc.,
- Initialization of essential variables (`HOME`, `PATH`, `SHLVL`, `IFS`, `PWD`, `_`),
- Helpers for applying temporary assignment lists and building `char **envp` for exec.

The environment is directly integrated with `t_shell` via the `t_vec_env env` field. This ensures a single, consistent view of the environment for the entire shell.

---

## 2. Core Data Structures

### 2.1 `t_env` – single environment entry

Defined in `env.h` (used across this module):

```c
// ...existing code...
typedef struct s_env
{
    bool   exported;  // should this entry be visible to child processes?
    char  *key;       // variable name (e.g., "PATH")
    char  *value;     // variable value (e.g., "/usr/bin:/bin")
}   t_env;
// ...existing code...
```

Semantics:

- `key` and `value` are heap‑allocated C strings owned by the environment vector.
- `exported` controls whether the entry should be present in the `char **envp` given to `execve()` and similar.
- A `t_env` can represent both **shell variables** (non‑exported) and standard environment variables (exported).

### 2.2 `t_vec_env` – dynamic array of `t_env`

The environment is stored as a `t_vec_env` (a `t_vec` specialized for `t_env`):

```c
// ...existing code...
typedef t_vec  t_vec_env;   // vector with elem_size = sizeof(t_env)
// ...existing code...
```

Properties and expectations:

- `env.len` is the number of entries.
- `env.ctx` points to a contiguous array of `t_env` elements.
- Vector operations (`vec_init`, `vec_push`, `vec_pop`, etc.) are used to manage capacity and length.
- Entries are logically ordered but order is not semantically important.

### 2.3 `t_shell.env` – global environment for the shell

In `t_shell` (see `core/README.md`), `env` is used as the **canonical** environment state:

```c
// ...existing code...
typedef struct s_shell
{
    t_vec_env env;
    // ...existing fields...
}   t_shell;
// ...existing code...
```

- All environment‑related operations (builtins, expansion, initialization) act on this vector.
- When spawning a child process, `get_envp(state, exe_path)` converts `env` to `char **`.

---

## 3. Converting Between `envp` and `t_vec_env` (`conv.c`)

### 3.1 `str_to_env` – parse `KEY=VALUE` string

```c
// ...existing code...
t_env str_to_env(char *str)
{
    t_env   ret;
    char   *eq;
    size_t  keylen;

    eq = ft_strchr(str, '=');
    ft_assert(eq != 0);
    keylen = (size_t)(eq - str);
    ret.exported = true;
    ret.key = ft_strndup(str, keylen);
    ret.value = ft_strdup(eq + 1);
    return (ret);
}
// ...existing code...
```

Algorithm:

1. Find the first `=` in the string.
2. Split into `key` and `value` by position of `=`.
3. Duplicate both parts on the heap:
   - `key = str[0..eq-1]`
   - `value = eq+1 .. end`
4. Mark the entry as `exported = true` (since `envp` only contains exported variables).

Assumptions:

- Input is always valid `KEY=VALUE` – enforced by `ft_assert(eq != 0)`.

### 3.2 `env_to_vec_env` – initial environment construction

```c
// ...existing code...
t_vec_env env_to_vec_env(t_shell *state, char **envp)
{
    t_vec_env   ret;
    t_env       tmp;

    vec_init(&ret);
    ret.elem_size = sizeof(t_env);
    while (*envp)
    {
        tmp = str_to_env(*envp);
        vec_push(&ret, &tmp);
        envp++;
    }
    if (state->cwd.len)
        env_set(&ret, env_create(ft_strdup("PWD"),
                ft_strdup((char *)state->cwd.ctx), true));
    if (state->cwd.len)
        env_set(&ret, env_create(ft_strdup("IFS"),
                ft_strdup(" \t\n"), false));
    return (ret);
}
// ...existing code...
```

Responsibilities:

1. **Import parent `envp`** into the internal vector using `str_to_env`.
2. Ensure that **`PWD`** and **`IFS`** are present and consistent with shell expectations:
   - If `state->cwd.len` is set:
     - Override or create `PWD` with the shell’s cached `cwd`.
     - Define `IFS` (internal field separator) as `" \t\n"` if not present, marked non‑exported (`exported = false`).

Design goals:

- Normalize the environment at startup into a predictable internal representation.
- Decouple from the raw `envp` pointer and allow controlled mutation via `env_set`.

---

## 4. Environment Lookup and Mutation (`utils.c`, `helpers.c`)

### 4.1 `env_create` – convenience constructor

```c
// ...existing code...
t_env env_create(char *key, char *value, bool exported)
{
    t_env e;

    e.key = key;
    e.value = value;
    e.exported = exported;
    return (e);
}
// ...existing code...
```

This wraps the raw struct initialization and makes ownership explicit: the caller must pass heap‑allocated `key` and `value`.

### 4.2 `env_get` and `env_nget` – lookup

```c
// ...existing code...
t_env *env_get(t_vec_env *env, char *key)
{
    t_env   *curr;
    size_t   i;

    i = -1;
    while (++i < env->len)
    {
        curr = &((t_env *)env->ctx)[i];
        if (ft_strcmp(key, curr->key) == 0)
            return (curr);
    }
    return (0);
}
// ...existing code...

t_env *env_nget(t_vec_env *env, char *key, int len)
{
    t_env *curr;
    int    i;

    i = env->len - 1;
    while (i >= 0)
    {
        curr = vec_idx(env, i);
        if (ft_strncmp(key, curr->key, len) == 0
            && curr->key[len] == 0)
            return (curr);
        i--;
    }
    return (0);
}
// ...existing code...
```

- `env_get`: exact string match, forward scan.
- `env_nget`: prefix/length match, backward scan (used by expansion).

Design note:

- Backward scan in `env_nget` ensures that most recently set variables shadow earlier ones (important when environment is updated repeatedly).

### 4.3 `env_set` – insert or update

```c
// ...existing code...
int env_set(t_vec_env *env, t_env el)
{
    t_env *old;

    ft_assert(el.key != 0);
    old = env_get(env, el.key);
    if (old)
    {
        free(old->value);
        free(el.key);
        old->value = el.value;
        old->exported = el.exported;
    }
    else
    {
        if (vec_push(env, &el))
            return (0);
        else
            return (1);
    }
    return (0);
}
// ...existing code...
```

Algorithm:

1. Look up existing entry by key.
2. If found:
   - Free the old value,
   - Free the new key (we reuse the existing key string),
   - Replace `value` and `exported` flag.
3. If not found:
   - Append the new `t_env` to the vector.

Ownership rules:

- On update, the caller’s `key` is freed and the old key is kept.
- On insert, the `key` and `value` become owned by the vector.

This is important for builtins like `export`, which often build temporary `t_env` objects.

---

## 5. Expansion Logic (`expand.c`)

The expansion layer answers the question: *“Given a variable name, what is its value in the current shell state?”*

### 5.1 `env_expand_n` – core expansion

```c
// ...existing code...
char *env_expand_n(t_shell *state, char *key, int len)
{
    t_env *curr;

    if (ft_strncmp(key, "?", len) == 0 && len == 1)
        return (state->last_cmd_st);
    else if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
        return (state->pid);
    else if (len == 0)
        return ("");
    curr = env_nget(&state->env, key, len);
    if (curr == 0 || curr->key == 0)
        return (0);
    return (curr->value);
}
// ...existing code...
```

Special cases:

- `$?` → string form of the last command status (`state->last_cmd_st`).
- `$$` → PID of the shell (`state->pid`).
- Empty name (like `$""` or malformed cases) → empty string.

Otherwise, lookup is performed via `env_nget`, which supports name length information (useful when parsing from a larger string).

Return value semantics:

- Returns a pointer directly to the environment value (no duplication).
- Caller should **not free** the returned pointer.
- Returns `NULL` if the variable does not exist.

### 5.2 `env_expand` – convenience wrapper

```c
// ...existing code...
char *env_expand(t_shell *state, char *key)
{
    return (env_expand_n(state, key, ft_strlen(key)));
}
// ...existing code...
```

Used when the entire `key` string is exactly the variable name.

### 5.3 `env_extend` and `env_apply_cmd_assigns`

These helpers support shell features like `VAR=val cmd` (temporary assignments for a single command) and merging local environments.

```c
// ...existing code...
void env_extend(t_vec_env *dest, t_vec_env *src, bool export)
{
    t_env curr;

    while (src->len)
    {
        curr = *(t_env *)vec_pop(src);
        if (!curr.key)
            continue ;
        curr.exported = export;
        env_set(dest, curr);
    }
    free(src->ctx);
    vec_init(src);
}
// ...existing code...
```

- Moves all entries from `src` into `dest`, optionally marking them all as exported or not.
- Consumes `src`: vector is cleared and its storage freed.

```c
// ...existing code...
void env_apply_cmd_assigns(t_shell *state,
            t_executable_cmd *src, bool export)
{
    size_t  i;
    t_env  *el;

    if (!state || !src)
        return ;
    i = 0;
    while (i < src->pre_assigns.len)
    {
        el = &((t_env *)src->pre_assigns.ctx)[i];
        if (!el->key)
        {
            i++;
            continue ;
        }
        el->exported = export;
        env_set(&state->env, *el);
        el->key = NULL;
        el->value = NULL;
        i++;
    }
}
// ...existing code...
```

- Applies a vector of assignment `t_env` entries (`src->pre_assigns`) to the global environment.
- After each entry is inserted, `key` and `value` pointers in the source entry are nulled to avoid double free when `pre_assigns` is later cleaned up.

---

## 6. Initializing and Maintaining Essential Variables (`helpers.c`, `init_dft_env.c`, `utils2.c`)

### 6.1 Current working directory (`init_cwd`, `set_cwd`)

```c
// ...existing code...
void init_cwd(t_shell *state)
{
    char *cwd;

    vec_init(&state->cwd);
    cwd = getcwd(NULL, 0);
    if (cwd)
        vec_push_str(&state->cwd, cwd);
    else
        ft_eprintf(MSG_GETCWD_SHINIT);
    free(cwd);
}
// ...existing code...
```

- Initializes `state->cwd` with the result of `getcwd()` if available.
- On error, prints a diagnostic but leaves the vector empty.

`set_cwd` in the current code re‑calls `init_cwd` and `getcwd()`, but `init_cwd` is already used by `on()`; the key idea is that `state->cwd` always reflects the shell’s logical working directory.

### 6.2 `HOME`, `PATH`, `SHLVL`, `_`, `PWD`, `IFS`

`init_dft_env.c` and `helpers.c` ensure that essential vars exist and have sane values:

- `set_home(state)`:
  - If `HOME` is missing or empty, set it to `getcwd()` or a fallback like `TMP_DIR`.
- `set_path(state)`:
  - If `PATH` is missing or empty, set it to `DFT_PATH`.
- `set_shlvl(state)`:
  - Reads `SHLVL`, parses it as an int (with `ft_checked_atoi`), increments it or resets to `1`.
- `set_underscore(state)`:
  - Ensures `_` (ultimate argument) is set; uses `state->ctx` or `MINISHELL`.
- `ensure_essential_env_vars(state)` ties them together and ensures `PWD` exists:

```c
// ...existing code...
void ensure_essential_env_vars(t_shell *state)
{
    char *cwd;
    t_env *e;

    cwd = NULL;
    set_home(state);
    set_path(state);
    set_shlvl(state);
    set_underscore(state);
    e = env_get(&state->env, PWD);
    if (!e || !e->value || !e->value[0])
    {
        cwd = getcwd(NULL, 0);
        if (!cwd)
            cwd = ft_strdup(TMP_DIR);
        env_set(&state->env, env_create(ft_strdup(PWD),
                ft_strdup(cwd), true));
        free(cwd);
    }
}
// ...existing code...
```

- This function is called during core initialization (`on()`), after `env_to_vec_env`.
- Goal: a predictable environment even when parent `envp` is incomplete or weird.

### 6.3 `update_pwd_vars` – sync `PWD`/`OLDPWD` with `cwd`

```c
// ...existing code...
void update_pwd_vars(t_shell *state)
{
    t_env *pwd;

    pwd = env_get(&state->env, PWD_NAME);
    if (pwd == NULL)
        try_unset(state, OLDPWD_NAME);
    else
        env_set(&state->env, env_create(ft_strdup(OLDPWD_NAME),
                ft_strdup(pwd->value), pwd->exported));
    env_set(&state->env, env_create(ft_strdup(PWD_NAME),
            ft_strdup((char *)state->cwd.ctx), true));
}
// ...existing code...
```

This function (currently in `environment/utils2.c`, used by `cd`) keeps:

- `OLDPWD` = previous `PWD` (or unset if there was no `PWD`),
- `PWD` = `state->cwd` after a successful directory change.

This is essential for POSIX `cd` semantics (`cd -`, prompts, etc.).

---

## 7. Building `char **envp` for Exec (`utils.c`)

### 7.1 `env_to_str` – serialize `t_env` back to `KEY=VALUE`

```c
// ...existing code...
static char *env_to_str(t_env *e)
{
    t_string s;
    char     ch;

    vec_init(&s);
    s.elem_size = 1;
    vec_push_str(&s, e->key);
    ch = EQ;
    vec_push(&s, &ch);
    vec_push_str(&s, e->value);
    return ((char *)s.ctx);
}
// ...existing code...
```

- Builds a new `t_string` vector,
- Appends `key`, then `=`, then `value`,
- Returns the underlying `char *` (caller owns and must eventually free).

### 7.2 `get_envp` – materialize environment for a child

```c
// ...existing code...
char **get_envp(t_shell *state, char *exe_path)
{
    char   **ret;
    size_t   i;
    size_t   j;
    t_env   *e;

    (void)exe_path;
    ret = ft_calloc(state->env.len + 1, sizeof(char *));
    i = -1;
    j = 0;
    while (++i < state->env.len)
    {
        e = &((t_env *)state->env.ctx)[i];
        if (e->exported)
            ret[j++] = env_to_str(e);
    }
    return (ret);
}
// ...existing code...
```

Algorithm:

1. Allocate an array big enough for all entries + terminating `NULL`.
2. Iterate over the environment vector.
3. For each `exported` entry, serialize it via `env_to_str` and append to the result.
4. Return the array for use with `execve()`.

Design notes:

- `exe_path` is currently unused but could be used in the future for ctx‑specific env manipulations.
- Only `exported` entries are visible to children, matching POSIX behavior of `export`.

---

## 8. Why This Design Matters

### 8.1 Separation of concerns

- The environment is kept as a **first‑class data structure** (`t_vec_env`), not just raw `char **`.
- Builtins, expansion, and initialization operate on this structured view.
- Conversion to/from `char **envp` only happens at well‑defined boundaries (startup and exec).

### 8.2 Shell correctness and POSIX behavior

- Proper handling of `HOME`, `PATH`, `SHLVL`, `PWD`, `OLDPWD`, `_`, `IFS` is crucial for shell scripts to behave as expected.
- Expansion of `$?` and `$$` is implemented in terms of `t_shell`’s last status and PID, matching typical shell semantics.
- `exported` flags exactly control which variables are visible to children, mirroring POSIX `export`.

### 8.3 Extensibility

- New special variables can be added by extending `env_expand_n` with more cases (e.g. `$RANDOM`, `$UID`).
- New default environment rules can be implemented by enhancing `ensure_essential_env_vars`.
- Because everything funnels through `env_set`/`env_get`, the invariants and memory ownership remain consistent.

In short, the environment module forms the backbone of shell state communication: between parent and child processes, between builtins and the core, and between the user and the execution environment. Understanding these structures and algorithms is key when extending or debugging the shell’s behavior.
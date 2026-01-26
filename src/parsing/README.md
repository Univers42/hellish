# Parsing Module – Grammar, Algorithms, and AST Design

The parsing module turns a **flat token stream** (from the lexer) into a
structured **AST** that describes what the shell should do.

Where the lexer only knows about characters and operators, the parser knows
about:

- simple commands and assignments,
- pipelines (`cmd1 | cmd2 | ...`),
- command lists and control operators (`;`, `&&`, `||`, `&`),
- subshells (`( ... )`),
- process substitutions (`<(cmd)`, `>(cmd)`),
- redirections (including FD‑prefixed forms like `2>>file`),
- arithmetic contexts (`$(( ... ))`) for error reporting.

It builds an AST composed of `t_ast_node` objects and communicates with the
infrastructure via a small **status protocol**:

- `parser->res == RES_OK` – AST is valid and complete.
- `RES_MoreInput` – grammar is incomplete and needs more tokens.
- `RES_FatalError` – irrecoverable syntax error.

---

## 1. Top‑Level Flow

The entry point is:

```c
t_ast_node parse_tokens(t_shell *state, t_parser *parser, t_deque_tt *tokens);
```

Conceptually it does:

1. Assume success (`parser->res = RES_OK`).
2. Parse a **simple list** – a sequence of pipelines separated by `;`, `&`,
   `&&`, `||`, and newlines:
   
   ```c
   ret = parse_simple_list(state, parser, tokens);
   ```

3. If that succeeded, consume the trailing `TT_END` token (asserting that all
   tokens were used).
4. Run post‑processing passes on the AST:
   - `reparse_words(&ret);`
   - `reparse_assignment_words(&ret);`

   These refine raw `AST_WORD`/`AST_TOKEN` sequences into more specific
   structures (e.g. assignment words) for the expander.

5. Optionally print the AST via `print_ast_dot` if tracing is enabled.

The rest of the parser is a set of mutually‑recursive functions that mirror a
small, shell‑like grammar.

---

## 2. Core Grammar and AST Nodes

The grammar, simplified, looks like this:

- **simple_list** – left‑associative list of **pipelines** separated by
  `;`, `&`, `&&`, `||` and newlines.
- **pipeline** – one or more **commands** separated by `|`.
- **command** – either:
  - a **simple command** (words, redirects, process subs), or
  - a **subshell** (`( compound_list )`) plus optional trailing redirects.
- **simple command** – sequence of:
  - words,
  - redirections (`<`, `>`, `>>`, `<<`, etc.),
  - process substitutions (`<(cmd)` / `>(cmd)`).
- **compound_list** – sequence of pipelines separated by list operators
  (similar to simple_list, but used *inside* subshells).

The AST reflects this with node types like:

- `AST_SIMPLE_LIST`
- `AST_COMMAND_PIPELINE`
- `AST_COMMAND`
- `AST_SIMPLE_COMMAND`
- `AST_COMPOUND_LIST`
- `AST_SUBSHELL`
- `AST_REDIRECT`
- `AST_PROC_SUB`
- `AST_WORD` / `AST_TOKEN`

Each parser function:

- owns the creation of a specific node type,
- fills its `children` vector with subnodes or token nodes,
- updates `parser->res` and may push/pop entries in `parser->parse_stack`.

`parse_stack` records *open constructs* (e.g. pending `(`, `|`, `&&`) so that
when more input is requested the prompt can explain what is missing.

---

## 3. Simple Lists and Control Operators

### 3.1 `parse_simple_list`

A **simple list** is the top‑level and subshell body structure:

```c
cmd1 ; cmd2 && cmd3 || cmd4 &
```

`parse_simple_list`:

1. Initializes a `AST_SIMPLE_LIST` node (via `init_ast_node_children`).
2. Peeks the first token:
   - If it is `TT_ARITH_START`, delegates to arithmetic error handling.
   - If it is neither a valid simple command token nor `(`, reports an
     unexpected token.
3. Parses the **first pipeline** via `push_parsed_pipeline_child`, which wraps:
   
   ```c
   tmp = parse_pipeline(...);
   vec_push(&ret.children, &tmp);
   ```

4. Repeatedly parses **list operators** using `process_all_simple_list_ops`:
   - `parse_simple_list_op`:
     - uses `push_simple_list_op_token` to:
       - check if the next token is a list operator (`;`, `&`, `&&`, `||`),
       - push that operator onto `parser->parse_stack`,
       - append an `AST_TOKEN` child representing the operator.
     - uses `check_newlines_and_end` to:
       - skip newlines,
       - detect and handle end‑of‑input cases that require more input or are
         invalid (`;` at end of stream without following command).
     - finally parses another pipeline and appends it.

5. After there are no more list operators (status 1), it calls
   `handle_final_newline_or_end` to clean up an optional trailing newline and
   to error on stray tokens.

This function builds a **flattened, left‑associative** AST where operators and
pipelines sit in a single `children` array, which is later interpreted by the
executor/decomposer.

### 3.2 Compound lists

`parse_compound_list` is used for subshell bodies (the content inside `( ... )`):

- It is structurally similar to `parse_simple_list`, but tuned for use inside
  `(` and `)`.
- It rejects arithmetic starts (`TT_ARITH_START`) directly.
- It starts by parsing an initial pipeline.
- Then repeatedly calls `parse_compound_list_s` to process internal operators.

`parse_compound_list_s`:

- Peeks the next token and checks `is_compound_list_op` (semicolon, `&&`,
  `||`, newline, `&`).
- If not an operator, returns `true` to signal “no more ops here”.
- Otherwise:
  - consumes the operator and pushes it as a token child,
  - handles special cases where `;` or newline appears immediately before a
    closing `)` via `is_semicolon_or_newline_before_brace_right`,
  - pushes the operator onto `parse_stack`,
  - skips newlines,
  - handles `TT_END` by setting `RES_MoreInput` (expecting more input before
    closing `)`),
  - parses another pipeline and appends it.

This logic is what allows multiline subshell bodies with complex list
operators while still behaving like the top‑level simple list.

---

## 4. Pipelines and Commands

### 4.1 `parse_pipeline`

A pipeline is a sequence of commands separated by `|`:

```c
cmd1 | cmd2 | cmd3
```

Algorithm:

1. Create `AST_COMMAND_PIPELINE`.
2. Parse and append the **first command** via `push_cmd_parsed`, which calls
   `parse_command` and pushes the result.
3. Push `TT_PIPE` onto `parse_stack`.
4. Call `process_pipeline_pipes`:
   - While the next token is `TT_PIPE`:
     - consume it and any following newlines,
     - if `TT_END` appears, set `RES_MoreInput` and return (pipeline expects
       another command),
     - parse another command and append it.
5. Pop `TT_PIPE` from `parse_stack`.

This makes incomplete pipelines like `echo foo |` naturally request more
input, which the infrastructure turns into a `pipe` continuation prompt.

### 4.2 `parse_command`

A **command** is either:

- a simple command, or
- a subshell plus optional redirects.

Algorithm:

1. Build an `AST_COMMAND` node and inspect the first token.
2. Reject `TT_ARITH_START` early (using arithmetic error helpers).
3. If the first token is `(` (`TT_BRACE_LEFT`), call `handle_subshell_case`:
   - `parse_subshell` to build the subshell node.
   - Then, while the next tokens are redirects, parse them and append them.

4. Otherwise, call `handle_simple_command_case`:
   - `parse_simple_command` parses a sequence of words, redirects, and process
     substitutions into an `AST_SIMPLE_COMMAND` child.

In both branches, errors in child parsing propagate up via `parser->res`.

### 4.3 `parse_simple_command`

A **simple command** consists of:

- regular words (command name, arguments),
- redirections attached to the command,
- process substitutions.

Algorithm:

1. Create `AST_SIMPLE_COMMAND`.
2. Check the first token with `is_simple_cmd_token` (word, redirect, proc_sub);
   if not, it’s a syntax error.
3. In a loop (`while (ST_SCANNING)`):
   - Use `parse_and_push_simple_cmd_child`:
     - If next token is a word → `push_parsed_word`.
     - If next is a redirect → `push_parsed_redirect` and propagate status.
     - If next is a process substitution → `push_parsed_proc_sub`.
   - Break when none of these apply.

This builds a flat sequence of child nodes; later passes interpret the first
word as command name, early words as assignments, etc.

---

## 5. Redirections and Process Substitution

### 5.1 `parse_redirect`

`AST_REDIRECT` nodes capture one redirection operator plus its target:

1. Consume the redirect operator token (ensuring `is_redirect(tt)` is true).
2. Push it as a token child.
3. Peek the next token:
   - If it is a process substitution (`TT_PROC_SUB_IN` or `TT_PROC_SUB_OUT`),
     call `parse_proc_sub` and append the resulting `AST_PROC_SUB` node.
   - Else if it is a valid redirect target (word, quoted word, envvar),
     call `parse_word` to create an `AST_WORD` child.
   - Else, it’s an unexpected token.

`is_redirect_target` centralizes which token types can legally follow a
redirection operator.

### 5.2 `parse_proc_sub`

Process substitution constructs like `<(cmd)` and `>(cmd)` are parsed into
`AST_PROC_SUB` nodes that hold:

- The operator itself (`<(` or `>(`), as an `AST_TOKEN` child.
- A single `AST_WORD` child whose token contains a *copy* of the command
  substring that will later be run in a separate process.

The algorithm in `parser_proc_sub.c`:

1. Consume the operator token.
2. Use `collect_proc_sub_command` to:
   - maintain a *depth counter* over parentheses,
   - track the start and end pointers of the inner command text,
   - stop when the matching closing `)` is reached,
   - request `RES_MoreInput` via `proc_sub_handle_eof` if EOF arrives before
     closing the construct.

3. Once `cmd_start`/`cmd_end` are known:
   - duplicate that substring (`ft_strndup`),
   - wrap it in an `AST_WORD` → `AST_TOKEN` node via `create_word_node`,
   - append it as a child.

This design isolates the somewhat special semantics of process substitution in
one function, while exposing a very simple AST shape to the expander.

---

## 6. Subshells and Compound Lists

### 6.1 `parse_subshell`

Subshells have the form:

```sh
( compound_list )
```

Algorithm:

1. Create `AST_SUBSHELL`.
2. Push `TT_BRACE_LEFT` onto `parse_stack` (for prompt/debug context).
3. Ensure the current token is `TT_BRACE_LEFT`; otherwise call `unexpected`.
4. Consume `(`.
5. Parse a **compound list** via `push_parsed_compound_list`.
6. If parsing failed, return immediately.
7. Require the next token to be `TT_BRACE_RIGHT`; if not, call `unexpected`.
8. Pop `)` token from the deque, pop `TT_BRACE_LEFT` from `parse_stack`,
   and return the node.

This nicely composes with `parse_command` and list operators.

---

## 7. Arithmetic Context Errors

While full arithmetic parsing lives elsewhere, the parser provides **good
error messages** when arithmetic constructs are malformed.

If a `TT_ARITH_START` appears where a list or command is expected,
`handle_arith_error` is invoked:

- It consumes the arithmetic token and then uses
  `handle_arith_error_collect` to:
  - walk tokens until matching closing `)` depth is reached or `TT_END`,
  - track inner parentheses (flagging `has_inner_paren`),
  - remember the last `TT_WORD` seen (`last_word`),
  - store relevant fragments into a `t_string expr_buf` (for debug/text).

- `handle_arith_error_print` then prints errors like:

  - “missing `)` (error token is `"..."`)” when there are unmatched parens.
  - “syntax error in expression (error token is `"..."`)” otherwise.

- The parser res is set to `RES_FatalError` and the shell status to `1`.

This separation keeps the main grammar simple while still yielding descriptive
error messages for `$(( ... ))` misuse.

---

## 8. Helper Functions and Parse Stack

Many small helpers encapsulate common tasks:

- **Token classification**:
  - `is_redirect`, `is_proc_sub`, `is_simple_cmd_token`,
    `is_simple_list_op`, `is_compound_list_op`,
    `is_end_token`, `is_newline_token`.

- **AST node creation**:
  - `init_ast_node_children` – initialize a node with a given type.
  - `push_token_child` – append a token as an `AST_TOKEN` child.
  - `add_op_token_child` – operator‑specific variant.
  - `parse_word` – wrap a single token as `AST_WORD` → `AST_TOKEN`.

- **Push wrappers** (for readability):
  - `push_parsed_word`, `push_parsed_redirect`, `push_parsed_proc_sub`,
    `push_parsed_compound_list`, `push_cmd_parsed`, `push_parsed_pipeline_child`.

- **Error propagation**:
  - `handle_unexpected_token` – wraps `unexpected` and sets `parser->res`.
  - `check_res_ok` – convenience test for `parser->res == RES_OK`.

- **Parse stack operations**:
  - `vec_push_int(&parser->parse_stack, op)` to remember open operators.
  - Higher levels (in the infrastructure) use this to build more‑input
    prompts like `pipe>`, `cmdand>`, `subsh>`, etc.

This style follows the same philosophy as the lexer: **lots of small, focused
helpers** instead of one monolithic function.

---

## 9. Interaction with Other Modules

- **Lexer**:
  - Parser consumes tokens from `t_deque_tt` produced by `tokenizer`.
  - It never attempts to “pull” more tokens directly; that is handled by the
    infrastructure (`get_more_input_parser`).

- **Infrastructure / input**:
  - The main loop checks `parser->res` after each parse attempt:
    - `RES_MoreInput` → build a continuation prompt and gather more tokens.
    - `RES_FatalError` → print syntax errors and discard the incomplete AST.

- **AST / decomposer / expander**:
  - Once `RES_OK`, the AST is stored in `state->tree` and later traversed by
    decomposer/expander logic to construct executable commands.
  - Post‑processing (`reparse_words`, `reparse_assignment_words`) refines
    word nodes before expansion.

---

## 10. Design Goals

The parser is intentionally:

- **Hand‑written and explicit** – no parser generators; everything is in C
  functions that mirror the grammar.
- **Layered** – simple_list → pipeline → command → simple_command → word /
  redirect /
  proc‑sub.
- **Error‑aware** – carefully distinguishes “need more input” from “hard
  syntax error”, allowing interactive prompts to feel natural.
- **AST‑centric** – every function’s main job is to build an AST node with a
  clean child structure, leaving execution semantics to later phases.

This makes it practical to extend the grammar (e.g. more list operators,
additional compound forms) and to debug misparses by inspecting the AST or
parsing traces, while keeping a clear boundary between lexing, parsing,
expansion, and execution.
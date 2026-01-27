# Word Splitting / Reparser Module – Conceptual Documentation

This module sits **between parsing and expansion**. The parser initially
creates relatively coarse `AST_WORD` nodes that each contain a single
`AST_TOKEN` with raw text (including quotes, `$`, backslashes, etc.).

Before the expander can apply shell rules (word splitting, parameter
expansion, quote removal), we need a more detailed structure that knows where
quotes begin/end, where `$VAR` segments live, and how assignments are shaped.

The **reparser** performs this refinement:

> It walks the AST and restructures `AST_WORD` nodes into smaller sub‑tokens
> that explicitly encode quoting and variable boundaries, and it marks
> assignment words separately.

This mirrors what many shells do internally: they separate *syntactic words*
(from the parser) from *expansion units* (for the expander).

---

## 1. High‑Level Role

Starting point:

- The parser produces nodes like:
  - `AST_WORD` with one `AST_TOKEN` of type `TT_WORD` whose `start`/`len` cover
    the entire original lexeme.

Goal of this module:

- Replace those coarse `AST_WORD` nodes with a **tree of sub‑tokens**:
  - segments that are always literal (`TT_WORD`, `TT_SQWORD`, `TT_DQWORD`),
  - segments that represent environment variables (`TT_ENVVAR`, `TT_DQENVVAR`),
  - segments that represent quoted regions.
- Identify and re‑tag nodes that are **assignment words**:
  - transform a `WORD` like `NAME=value` into an `AST_ASSIGNMENT_WORD` that has
    a clear key (`NAME`) and value (`value`) subtree.

This makes it much easier for the expander to:

- Apply quote removal correctly.
- Decide when word splitting should or should not happen.
- Extract variable names and values for `VAR=val` assignments.

---

## 2. Core Data Structures

### 2.1 `t_interval`

A simple pair `{start, end}` (byte offsets) used to refer to slices inside a
`TT_WORD` token’s `start` buffer.

### 2.2 `t_reparser`

A small state struct used while reparsing a single word token:

```c
typedef struct s_reparser {
    t_ast_node current_node;   // AST being built from the original word
    int        i;              // current byte index in current_token
    t_token    current_token;  // original TT_WORD token
    t_tt       ctx_tt;     // ctx type (e.g. TT_WORD, TT_DQENVVAR)
    int        prev_start;     // start index of the current segment
} t_reparser;
```

Instead of building a new string, the reparser keeps using the original
`current_token` buffer and only creates **subtoken nodes** with proper
intervals.

### 2.3 Subtoken nodes

`push_subtoken_node` creates a new `AST_TOKEN` child that:

- points into the same `start` buffer as the original word token,
- but with a smaller interval `[offset.start, offset.end)`,
- and a more specific `t_tt` tag (`TT_SQWORD`, `TT_DQWORD`, `TT_ENVVAR`, ...).

These subtokens live inside an `AST_WORD` (or later `AST_ASSIGNMENT_WORD`)
node and are used by the expander.

---

## 3. Re‑Parsing WORD Nodes

### 3.1 Top‑level entry: `reparse_words`

`reparse_words(t_ast_node *node)` walks the AST and refines every
`AST_WORD` node it finds:

- If `node->node_type == AST_WORD`:
  - It expects exactly one child `AST_TOKEN`.
  - It creates a `t_token_old full_word` that remembers the original full
    lexeme (`start`/`len`, `allocated` flag).
  - It calls `reparse_word(tok)` to build a new `AST_WORD` whose children are
    subtokens.
  - It ensures all children carry a `full_word` field pointing back to the
    original logical word.

- For non‑`AST_WORD` nodes (except `AST_PROC_SUB`), it recurses into children
  and re‑parses any nested words they contain.

This produces an AST where each logical word is now a **structured sequence**
of smaller units.

### 3.2 `reparse_word` – main tokenizer for words

`reparse_word(t_token t)` is the core routine:

- Initializes a fresh `AST_WORD` node with no children.
- Sets up a `t_reparser` pointing at this node and the single `TT_WORD` token.
- Calls `loop_node_rp(&rp)`:

  ```c
  while (rp.i < rp.current_token.len) {
      if (char == '"')        reparse_dquote(...);
      else if (char == '\\') reparse_bs(...);
      else if (char == '\'')  reparse_squote(...);
      else if (char == '$')    reparse_envvar(..., TT_ENVVAR);
      else if (is_space(char)) assert(false); // spaces never appear inside a TT_WORD
      else                     reparse_norm_word(...);
  }
  ```

Each helper consumes some bytes from `current_token` and appends a subtoken
node to `current_node`.

This split mirrors the lexer’s character‑wise logic, but working *inside a
single word token* and encoding semantics in AST children rather than in token
streams.

---

## 4. Handling Quotes and Escapes

### 4.1 Single quotes – `reparse_squote`

Single‑quoted strings are taken **literally**:

- The helper scans from the opening `'` to the matching closing `'`.
- It creates one `TT_SQWORD` subtoken covering only the content inside the
  quotes (not the quote characters themselves).

This separately tracks the literal part, so the expander knows this region is
immune to `$` expansion and word splitting.

### 4.2 Double quotes – `reparse_dquote`

Double quotes are more complex: they allow `$` expansion and some escapes.

Algorithm (simplified):

1. Assert and skip the opening `"`.
2. Initialize a `t_reparser` for the current token.
3. Loop until the closing `"`:
   - When a backslash is seen, flush any preceding literal segment and call
     `process_escape_seq_rp`.
   - When a `$` is seen, flush preceding literal and call
     `process_dollar_in_dquote_rp`, which internally uses `reparse_envvar` with
     a `TT_DQENVVAR` ctx.
   - Otherwise, move forward.
4. At the end, flush any trailing literal segment as a `TT_DQWORD`.
5. If nothing at all was pushed (empty quotes), insert an empty `TT_DQWORD`
   subtoken.

Escapes inside double quotes (`process_escape_seq_rp`) implement POSIX rules:

- `\"`, `\$`, `\\`, ``\``` are treated as a single escaped character.
- `\
` (backslash + newline) is removed completely.
- Other escapes are passed through as literal backslash+char.

### 4.3 Standalone backslash – `reparse_bs`

Outside quotes, a `\` used as an escape becomes:

- Either `TT_SQWORD` over the last character (including the backslash) when
  there is nothing after it,
- Or over the escaped one‑byte sequence.

This allows the expander to treat the escaped character as literal and to know
that the backslash itself should be removed.

### 4.4 Normal word segment – `reparse_norm_word`

For runs of characters that are not special (`$`, quotes, backslash, space),
`reparse_norm_word` creates a plain `TT_WORD` subtoken over that contiguous
segment.

---

## 5. Environment Variables and Parentheses

Environment references are introduced by `$` and may have several forms:

- `$FOO`, `$FOO_BAR` (normal identifiers),
- `$?`, `$$` (special variables),
- `$(( ... ))` arithmetic ctxs,
- `$(` or `${...}`‑like constructs (handled by other stages when present).

`reparse_envvar` is the central helper:

1. Consumes the initial `$`.
2. Initializes a `t_reparser` with a ctx type (`TT_ENVVAR` or
   `TT_DQENVVAR` if inside double quotes).
3. Checks two special cases:
   - **Parenthesized constructs** via `try_handle_paren_rp` and
     `reparse_envvar_paren`:
     - Handles `$((...))` style arithmetic by tracking parenthesis depth,
       including nested `((` and `))`.
   - **Special variables** via `try_handle_special_rp` and
     `reparse_special_envvar` (e.g. `$?`, `$$`).

4. Otherwise, consumes a valid identifier via `consume_ident_rp` (using
   `is_var_name_p1`/`is_var_name_p2`).
5. If no identifier was consumed at all:
   - handle empty/special cases via `handle_envvar_empty`:
     - maybe treat `'$'` followed by a quote or other literal as just that.

6. If an identifier exists, `handle_envvar_ident` creates a subtoken in the
   appropriate `tt` (plain `TT_ENVVAR` or `TT_DQENVVAR`).

`reparse_envvar_paren` delegates parenthesis depth tracking to helpers like
`is_double_open_paren`, `is_double_close_paren`, `is_open_paren`, and
`is_close_paren`, so the arithmetic region is captured as a single subtoken.

---

## 6. Assignment Word Re‑tagging

Shell semantics distinguish between:

```sh
FOO=bar cmd         # assignment word
FOO=bar             # pure assignment; no command name
FOO=bar=baz         # still one word, but only first '=' matters for the key
```

`reparse_assignment_words` walks the AST **after** `reparse_words` and:

- Skips `AST_PROC_SUB` to avoid changing embedded script text.
- Recursively visits children elsewhere.
- For each `AST_WORD` node, calls `reparse_assignment_word`:
  - It finds the first subtoken’s `t_token` (first child must be `AST_TOKEN`).
  - It finds the first `=` inside the token text (`find_eq_pos`).
  - It verifies that everything before `=` is a **valid identifier** via
    `is_valid_ident` (using variable name rules).
  - It then calls `apply_assignment_split`:
    - Creates a new root node `AST_ASSIGNMENT_WORD`.
    - Creates a subtoken covering `[0, eq)` as `TT_WORD` (the key).
    - Adjusts the original token to start at `eq + 1`, with shortened len.
    - Appends the original `AST_WORD` node (now representing just the value)
      as a child of the new `AST_ASSIGNMENT_WORD`.

The result is that assignment words now have a clear **key/value structure** in
the AST, which the expander and environment module can use to build `t_env`
entries.

---

## 7. Why This Module Exists (and Why It’s Structured Like This)

The lexer and parser already handle quoting and operators, so why reparse?

Because the **needs of expansion** are different from the grammar’s needs:

- Expansion cares about **fine‑grained boundaries**: where strings become
  literal vs expandable; where a variable name stops; what is inside quotes.
- The parser would become much more complex if it tried to express all this in
  the main grammar.

By having a dedicated reparser:

- The parser stays simple, dealing only with token types and high‑level
  constructs.
- The reparser can focus on **per‑word micro‑syntax**, staying localized to
  `word_splitting/`.
- The resulting AST is rich enough for the expander to:
  - easily apply shell rules (quote removal, splitting, parameter expansion),
  - correctly interpret assignment words.

Because all this logic is funneled through small helpers (`reparse_squote`,
`reparse_dquote`, `reparse_envvar`, `reparse_assignment_word`, etc.), it is
relatively straightforward to extend with:

- new parameter forms,
- new quoting rules,
- or additional tagging (e.g. distinguishing arithmetic envvars) without
  touching the parser or lexer.

In short, the word‑splitting/reparser module is a **bridge** from the parser’s
coarse AST to the expander’s fine‑grained needs, encoding all the tiny
per‑character semantics that make shell words behave the way users expect.
# Lexer Module

The lexer is the **first semantic step** after raw input. It turns a byte
string into an ordered stream of tokens (`TT_WORD`, `TT_PIPE`, `TT_HEREDOC`,
`TT_AND`, ...) that the parser consumes. The design is deliberately small
and hand-written: a cursor over a C string, tiny *advance* helpers for
quotes and nested constructs, one word machine, and a clear contract with
the layer above -- "I give you tokens, you tell me when you need more input".

Public API and structs: `incs/lexer.h`; token types: `incs/public/token.h`.

---

## 1. Role and contract

```c
char	*tokenizer(char *str, t_deque_tok *ret);
```

- Scans `str`, which the input layer owns and must keep alive as long as the
  deque: tokens are *slices*, not copies.
- Clears `ret->deqtok`, sets `ret->base = str`, fills the deque, and always
  terminates it with a `TT_END` token.
- Returns `NULL` when the stream is complete, or a continuation prompt
  (`"squote> "`, `"dquote> "`, `"subshell> "`, `"bquote> "`, `"param> "`,
  `"quote> "`) when the input ends inside an unterminated construct, also
  recording the missing closer in `ret->looking_for`.

`lex_line()` (`tokenizer2.c`) is the resumable variant for the streaming
path: it lexes one logical line at a time (stops after a top-level
`TT_NEWLINE`), neither clears the deque nor pushes `TT_END`, and stores
offsets against the whole cycle buffer so a construct spanning several calls
keeps coherent slices.

The parser then reports `RES_OK`, `RES_GETMOREINPUT` or `RES_ERR`; the lexer
never touches AST nodes.

---

## 2. Token model

`t_token` (`incs/public/token.h`) is `tt` (a `t_tt` packed into one byte),
`allocated`, `split_eligible`, `len`, `start`, plus two back-references the
parser and expander use (`full_word`, `arith_cache`). Packing keeps it at 32
bytes; it is embedded in every AST node.

Inside the deque the slot is the leaner `t_ltoken`: a 32-bit `off` from
`t_deque_tok.base` instead of a pointer, `len` in 24 bits, `tt` in 7. That
halves the slot (a 50k-line parse holds ~310k tokens) and works because the
lexer never sets `allocated` -- only the expander does, on AST tokens. The
conversion lives in exactly two inline helpers: `push_ltok()` packs on push
(`tok2ltok`), `pop_tok()` lifts on pop (`ltok2tok`).

`t_deque_tok` is `{ deqtok, looking_for, base }`.

The lexer emits `TT_WORD` for every word; the finer variants in `t_tt`
(`TT_SQWORD`, `TT_DQWORD`, `TT_ENVVAR`, `TT_DQENVVAR`) are assigned later,
when the expander re-lexes a word. Keyword types (`TT_IF`, `TT_DO`,
`TT_LBRACE`, ...) come from a second pass (section 5).

---

## 3. Operators (`helper3.c`, `helper4.c`)

Operator recognition used to walk a table on every call, with dozens of
`ft_strlen`/`ft_strncmp` per token; it is now a **first-character dispatch**.
`parse_op()`:

1. tries `check_fd_redirect()` (`helper4.c`) for the fd-prefixed forms
   (`2>`, `10<`, `3>&`, `2>>`), with `fd_redir_type` picking the longest
   form first;
2. otherwise routes on the leading byte to `op_left` (`<` family: `<<<`,
   `<<-`, `<<`, `<(`, `<&`, `<>`, `<`), `op_right` (`>>`, `>(`, `>&`, `>|`,
   `>`), or `op_other` (`||`, `|`, the `&` family via `op_amp` -- `&&`,
   `&>>`, `&>`, `&` -- `;;&`, `;;`, `;&`, `;`, `((`, `(`, `)`, and zsh's
   `=(` as `TT_PROC_SUB_FILE`, gated on the dialect).

Each matcher checks the longest spelling first (POSIX maximal munch), and
`ft_assert(len > 0)` guards the invariant that `is_word_boundary` never
routes a plain word byte here. Reading `&>` byte by byte once made
`cmd &>f` a background job with stderr on the terminal; that is why `op_amp`
exists.

---

## 4. Words, quotes, and nested spans

### 4.1 Boundaries (`helper2.c`)

`is_word_boundary(s)` consults a 256-entry character-class table (`g_cl`:
metacharacter / blank / digit bits) instead of an `ft_strchr` walk -- these
predicates run over a million times on a large parse. A word ends on a
metacharacter, a blank (space or tab only; newline is a token), or a
fd-redirect start (`is_fd_redirect_start`: one or two digits then `<`/`>`),
so `echo2>file` is `echo` + `2>` + `file`. `is_space` and
`is_special_char` read the same table.

### 4.2 The word machine (`parse_lexeme.c`)

`parse_lexeme(tokens, &str)` remembers `start` and loops over
`handle_next_chunk`, which tries, in priority order:

- `$'...'` -> `advance_ansic` (`lexer_advance2.c`; escapes are live, so
  `\'` does not close it);
- `handle_special`: `$(` -> `tokenize_subshell`; backtick ->
  `advance_backtick`; `${` -> `advance_brace_param`;
- `parse_generic`: a backslash pair (`advance_bs`) or any non-boundary byte;
- `'` / `"` -> `parse_quote` (`helper5.c`) -> `advance_squoted` /
  `advance_dquoted` (`lexer_advance.c`).

Each helper returns a status; on unterminated input the chunk handler sets
`tokens->looking_for` and hands back the prompt string, and `parse_lexeme`
returns it without pushing a token. Otherwise `push_word_token` emits one
`TT_WORD` from `start` to the stop point -- a slice, no copy.

`advance_dquoted` recurses into `$(...)`, `${...}` and backticks, because a
`"` inside them is not the outer quote's end (autoconf's `x="`... "" ...`"`,
and bash-completion's `"${u:-"a b"}"`). `advance_brace_param` tracks `${`
nesting only, so a stray `{` inside `${x:-a{b}` does not deepen it.

Two zsh-only breaks are gated on the dialect: `word_group_ahead`
(`lex_extglob.c`) swallows an extglob group `@(a|b)` or a zsh glob
qualifier `(DN)` (`glob_qual_ahead`, `lex_glob_qual.c`) *into* the word so
its `(` does not open a subshell, and `zsh_eqsub_break` ends a word before
a second `=` so `f==(:)` lexes as `f=` plus `=(`.

### 4.3 Subshells (`parse_subshell.c`)

`tokenize_subshell` consumes a `$(...)` span without tokenising its
interior; the expander re-lexes it later. Nesting, backslashes and quoted
spans are honoured, and the paren depth itself is owned by the shared
`casescan` automaton (`incs/casescan.h`, `src/helpers/casescan.c`, issue
#95): a `case` pattern's closing `)` is unbalanced by design, and every
scanner that just counted parens ended the substitution at the first
pattern. If the `)` never arrives, `looking_for = ')'` and `"subshell> "`
is returned.

---

## 5. The top-level loop and the keyword pass

`tokenizer()` (`tokenizer.c`) loops `skip_noise` (comments, and
backslash-newline continuations -- removed here rather than tokenised, or a
sourced function body would gain a spurious argument) then `tokenize_step`:

1. `[[ ... ]]` handling: `db_regex_word`, `dbracket_toggle`,
   `db_track_regex`, `db_newline_skippable`, `emit_dbracket_word`
   (`dbracket_lex.c`, `dbracket_lex2.c`). Inside `[[ ]]`, `&&`, `||`, `(`,
   `)`, `<` and `>` are emitted as `TT_WORD` so the conditional is not split,
   and the extglob cell is armed for the operand (issue #105).
2. `try_parse_lexeme` -> `parse_lexeme` for anything that starts a word
   (quote, `$`, an extglob group, or any non-boundary byte);
3. `\n` -> `emit_newline` (`TT_NEWLINE` is a real token: the grammar uses it
   as a separator inside compound commands);
4. blank -> skip; otherwise `parse_op`.

Keywords are **not** decided during tokenisation. `reclassify_keywords(tokens,
zsh)` (`keywords2.c`) is a second, linear pass the input layer runs after
`tokenizer()` (see `try_parse_tokens` in `src/infrastructure/input_utils4.c`
and the `exec_string*` paths): it tracks command position (`is_cmd_position`,
`keywords.c`), skips redirect targets and the `for` variable name, latches
one extra position after `function NAME` (`is_function_kw`) and `coproc`,
and upgrades a `TT_WORD` in command position via `reclassify_word` ->
`match_kw_part1`/`match_kw_part2`. `keywords_zsh.c` adds the dialect cases:
`} always {` (`is_always_kw`) and a `}` that closes a group without being at
command position (`brace_step`, what oh-my-zsh's one-line functions need).
Two passes keep the tokenizer context-free and the keyword rules in one
place.

---

## 6. Debug tooling

`--debug=lexer` runs `debug_lexer_loop` (`src/infrastructure/
input_get_more.c`), which never parses and just prints the token stream:

- `get_tt_names()` / `get_color_map()` (`singletons.c`, `singletons_kw.c`)
  are lazily built name and ANSI-colour tables; `tt_to_str()` and
  `token_color()` (`debug.c`) read them.
- `visible_lexeme_len()` and `print_visible_lexeme_noquotes()` (`debug.c`)
  render `\n`/`\t` as two characters so columns line up.
- `compute_columns()` (`tables_utils.c`), `print_table_header()` /
  `print_table_footer()` (`tables.c`) and `print_tokens()`
  (`print_tokens.c`) draw the table. `get_token_display_name()`
  (`print_tokens_utils.c`) shows a fully quoted word as `TOKEN_DQ` /
  `TOKEN_SQ` (`incs/sys.h`) instead of `TT_WORD`, which makes quoting
  mistakes visible at a glance.

Adding a token type means updating `t_tt`, the name table and the colour
map; the debug table needs nothing else.

---

## 7. Interaction with the rest of the shell

The **input layer** (`src/infrastructure/`) fills `state->input` via
`readline_cmd` / `buff_readline`, splices aliases into `state->alias_exp`,
calls `tokenizer()`, drops empty streams (`is_empty_token_list`), extracts
heredoc bodies, runs `reclassify_keywords()`, and only then calls the
parser. The **parser** pops with `pop_tok()` and keeps its own bookkeeping
in `parser->parse_stack`; when it returns `RES_GETMOREINPUT` the prompt
comes from the parser, not the lexer.

The lexer exposes nothing else: no AST, no grammar, no precedence -- just a
categorised, slice-based stream and one byte saying what it is waiting for.

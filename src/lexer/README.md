# Lexer Module – Conceptual Documentation

The lexer is the **first semantic step** after raw input. It turns a plain byte
string into a structured stream of tokens (`TT_WORD`, `TT_PIPE`, `TT_HEREDOC`,
`TT_ENVVAR`, etc.) that the parser can consume.

The design here was strongly inspired by how the **Crast interpreter** handles
lexing:

- A small, focused lexer that works on a *simple cursor over a C string*.
- Explicit, hand‑written logic for quotes, subshells, and operators.
- A clear **contract** with the parser: “I give you an ordered token stream,
  you tell me when you need more input.”
- Rich **debug tooling** to inspect tokens visually.

The goal is a lexer that is **predictable**, easy to debug, and tolerant of
incomplete input (multiline commands, open quotes, unfinished subshells).

---

## 1. Role and Contract

From the parser’s perspective, the lexer is a function:

> `tokenizer(char *input, t_deque_tt *out) -> optional prompt string`

It:

- Scans the `input` buffer owned by the infrastructure layer.
- Fills `out->deqtok` with `t_token` objects terminated by a `TT_END` token.
- Sets `out->looking_for` when the input ends in an *incomplete construct*
  (unclosed quote / subshell), so the infrastructure can build a more‑input
  prompt.
- Returns `NULL` when the token stream is complete, or a small prompt string
  like `"quote> "` / `"arith> "` when more input is required.

The parser then decides whether:

- It can parse a full command (`RES_OK`),
- It needs more tokens (`RES_MoreInput`),
- Or the stream is syntactically invalid (`RES_FatalError`).

The lexer never allocates or frees AST nodes; it only manipulates `t_token`
structures and simple C pointers.

---

## 2. Token Model and Operator Table

### 2.1 `t_token` and `t_tt`

Each token is:

- A **type** `t_tt` (token tag), e.g. `TT_WORD`, `TT_PIPE`, `TT_HEREDOC`,
  `TT_ENVVAR`, `TT_DQWORD`, etc.
- A `start` pointer into the original input buffer.
- A `len` (number of bytes).
- A small `allocated` flag for cases where the token owns its text.

Using slices instead of copies follows the Crast style: parsing is done
entirely in terms of *views* into the original string, with minimal
allocation.

### 2.2 Operator recognition via tables

Shell operators are recognized by a small **op table** (`t_op_map`) and a
“longest match” search:

- `helper3.c` defines arrays of `{string, token_type}` mappings:
  - group1: `|`, `<<`, `<<-`, `<`, `(`, `((`, …
  - group2: `)`, `<(`, `>(`, `>>`, …
  - group3: `>`, `&&`, `&`, `||`, `;`, `>&`, `<&`, …

- `longest_matching_str` scans this table and returns the operator whose
  string is the longest prefix of the current input position.

- `parse_op` then:
  - first checks for **FD‑prefixed redirects** (`2>`, `3>>`, `10<`, etc.) via
    `check_fd_redirect`,
  - else looks up the operator and emits a token of the right type.

This “longest match” approach is similar to Crast’s operator handling:
centralized, data‑driven, and avoids a big chain of `if`/`else if` tests.

---

## 3. Word Lexing, Quotes, and Subshells

The lexer’s core is **word lexing**: everything that isn’t whitespace, comment,
newline, or an operator is treated as part of a `TT_WORD` or related token.

### 3.1 Word boundaries

`is_word_boundary` decides when a word should end. It returns true if:

- The current character is a shell **special** (`|`, `>`, `<`, `&`, `;`, etc.),
- Or it starts a FD redirect (`2>`),
- Or it’s plain whitespace (`' '`, `\t`).

Anything else is considered part of a word, including `$`, letters, digits,
`_`, and most punctuation. This yields the usual shell behavior where `foo=bar`
or `$HOME/bin` are single lexemes.

### 3.2 Quotes

Quotes are handled using **small, dedicated advance functions**:

- `advance_squoted` walks until the matching `'` or end of string.
- `advance_dquoted` walks until the matching `"`, but treats `\"` specially
  (escaped double quotes in double‑quoted segments).

These follow the Crast style: *advance functions* move `char **str` forward
until a matching terminator or error state, and return a status code instead of
calling the parser.

`parse_quote` wraps both and also sets `tokens->looking_for` and returns a
prompt string (`LEXER_SQUOTE_PROMPT`, `LEXER_DQUOTE_PROMPT`) if an unclosed
quote is found. The infrastructure layer will then prompt the user for more
input with a suitable secondary prompt.

### 3.3 Subshells: `$(...)` with nesting

`parse_subshell.c` implements `tokenize_subshell`, which consumes a full
`$(...)` segment inside a word:

- When `parse_lexeme` sees `"$("`, it calls `tokenize_subshell`.
- `tokenize_subshell` uses a simple **depth counter**:
  - It increments depth on `(`,
  - Decrements on `)`,
  - Stops when depth returns to 0.
- It also delegates to `advance_bs` and `parse_quote` so that escaped or quoted
  parentheses do not disturb the balancing.

This is similar to how Crast handles nested constructs: minimal state, small
helper, no recursion into the full parser at lexing time.

If a subshell is never closed before the end of input, `parse_quote`/prompt
mechanism kicks in, causing `tokenizer` to return a more‑input prompt rather
than emitting broken tokens.

### 3.4 `parse_lexeme` – the word machine

`parse_lexeme` is the central state machine for a word:

- It remembers `start = *str`.
- Repeatedly calls `handle_next_chunk`, which:
  - handles subshell `$(...)`,
  - handles backslashes / word continuation via `parse_generic`,
  - handles entering quotes (`parse_quote`).
- Stops when a word boundary or error is seen.
- Emits a single `TT_WORD` from `start` to the stopping point.

This mirrors Crast’s practice of having a single **word parser** that delegates
special cases to tiny helpers, keeping the main loop readable and easy to
extend.

---

## 4. Comments, Newlines, and End‑of‑Stream

`tokenizer` is the top‑level function that orchestrates lexing a full input
buffer:

1. It clears the token deque and resets `prompt = NULL`.
2. Uses a simple `while (str && *str)` loop to process the input.
3. For each step:
   - If the current char is `#`, it uses `skip_shell_comment` to skip to the
     end of line.
   - Else it tries to parse a word via `try_parse_lexeme`:
     - if that returns a prompt string (incomplete construct), it stops and
       returns it.
   - Else if it sees a `\n`, it emits a `TT_NEWLINE`.
   - Else if it sees whitespace, it just advances.
   - Otherwise, it parses an operator via `parse_op`.
4. Finally, it emits a terminating `TT_END` token.

The parser and input infrastructure treat `TT_NEWLINE` and `TT_END` specially
(e.g. `is_empty_token_list`), but the lexer itself stays agnostic.

---

## 5. Debugging and Visualization

Inspired by Crast’s emphasis on visibility, this lexer includes a rich set of
**debug tools**:

### 5.1 Token names and colors

- `get_tt_names` lazily initializes an array mapping `t_tt` values to constant
  strings like `"TT_PIPE"`, `"TT_HEREDOC"`.
- `get_color_map` builds a small hash table that maps token names to ANSI color
  codes (`ASCII_GREEN` for words, `ASCII_YELLOW` for redirections, etc.).

`token_color(tt)` uses these to pick a color for each printed token type.

### 5.2 Visible lexeme length and printing

To print readable debug tables, the lexer defines:

- `visible_lexeme_len` – counts how many characters will be *displayed* when a
  lexeme is printed, treating `\n` as `"\n"`, `\t` as `"\t"`, etc.
- `print_visible_lexeme_noquotes` – prints the lexeme with those escapes.

This allows dynamic column width calculation:

- `compute_columns` scans all tokens, computing:
  - max width of type name column,
  - max width of length column (by counting digits),
  - max width of lexeme column (using `visible_lexeme_len`).

### 5.3 Pretty tables

- `print_table_header` / `print_table_footer` draw an ASCII/Unicode frame.
- `print_tokens` uses all of the above to print each token row as:

  - type (colored),
  - length, right‑aligned,
  - lexeme, escaped and padded.

There is also special logic in `get_token_display_name` for distinguishing
plain `TT_WORD` from quoted versions (e.g. `TOKEN_DQ` for double‑quoted
words), making debug output more informative.

This table output is used by the **debug lexer loop** in the infrastructure
layer (`debug_lexer_loop`), giving an experience similar to Crast’s rich
interactive debugging.

---

## 6. Interaction with the Rest of the Shell

The lexer intentionally stays **dumb about ASTs** and higher‑level constructs:

- It does not allocate AST nodes, it only fills a `t_deque_tt` with tokens.
- It doesn’t know about precedence or grammar rules; it simply categorizes
  lexemes and operators.
- It exposes minimal additional state (`looking_for` and an optional prompt
  string) to let the infrastructure build more‑input prompts.

The **input layer** (`get_more_tokens`) calls:

- `readline_cmd` / `buff_readline` to fill `state->input` with bytes,
- `tokenizer` to convert those bytes into tokens,
- uses the returned prompt/mode to decide whether to:
  - ask for more input (open quotes, partial lines),
  - or hand tokens over to the parser.

The **parser** then consumes tokens from `t_deque_tt` and pushes its own state
(operators, expected closures) into `parser->parse_stack`. When it needs more
input, it sets `RES_MoreInput`, which in turn affects the next prompt.

---

## 7. Why This Lexer Looks Like This

The design borrows several ideas from the Crast interpreter’s style:

- **Small helpers instead of giant state machines**:
  - quotes, subshells, backslashes, and operators are handled by tiny
    functions; `parse_lexeme` remains short and understandable.

- **Clear separation between lexing and parsing**:
  - the lexer is purely concerned with categorizing characters and sequences;
    it doesn’t try to enforce grammar rules.

- **Good tooling built‑in**:
  - colorized token tables and readable lexeme printing are first‑class,
    making it easy to see what the lexer is doing on tricky inputs.

- **Graceful handling of incomplete input**:
  - instead of erroring out on open quotes or partial subshells, the lexer
    signals that it needs more input and hands control back to the input
    service.

Because it is self‑contained and API‑like, the lexer can be extended fairly
safely:

- Adding a new operator mostly means adding an entry in the `t_op_map` table.
- Adding a new token type involves updating `t_tt`, `get_tt_names`, and the
  color map.
- Adjusting word boundaries or quote rules is localized to a few helpers.

Overall, the lexer is a **small, testable, and debuggable front end** between
raw input bytes and the parser’s notion of “tokens”, following the same
principles that made the Crast interpreter’s lexing strategy easy to reason
about and extend.
# Test Driven Development

we Want a robust, automateed way to test our lexer or any part of the shell even though the shell is a live runtime program. To create this environment, we need to separate concerns and make our shell testable.

# Principles to follow

- Decouple lexer from global state & OS interactions, make it pure function:
  input stiring -> token list (no forls, not termios, no global readline)
- lexer deterministic given teh same input and environment (so tests are reliable)
- data driven by using json (reasons: easy to write, easy to write new cases, easy to parse in many languages)

# filter pattern

<name>/_
<name>/\*\*
<name>/<name>
'_'
'**'
_feature
feature_
v[12].[0-9]+.[0-9]+
'_'
'_.jsx?'
'**.js'
docs/_
docs/**
docs/**/_.md
'**/dcs/**'
'**/README.md'
'**/_src/**'
'**/_-post.md'
'\*_/migrate-_.sql'
'_.md'
'!README.md'
'_.md'
'!README.md'
<name>/_
'README_'
<name>/_
<name>/_
<name>/_
<name>/_
<name>/\_
<name>/\*

# Lexer tests

- Runner uses: ./shell --debug=lexer -c "<input>"
- Runs non-interactively, only parses the debug block between markers.
- Run: make test-lexer
- Tests file: srcs/test/lexer_tests.txt

Each test is a single line:
input => TOKEN_A TOKEN_B TOKEN_C ...

Lexeme assertions:

- TOKEN_NAME(value) asserts exact lexeme equality for that token.

Ignoring whitespace:

- TOKEN_WHITESPACE is ignored by default.
- If you want to assert whitespace, include TOKEN_WHITESPACE in expected.

Ellipsis:

- Use ... at the end to ignore any extra trailing tokens.
  Example: echo x => TOKEN_IDENTIFIER TOKEN_IDENTIFIER ...

Examples:
echo "x" => TOKEN_IDENTIFIER(echo) TOKEN_DOUBLE_QUOTED_STRING(x) TOKEN_EOF
echo "hello\n" | wc -l => TOKEN_IDENTIFIER TOKEN_DOUBLE_QUOTED_STRING TOKEN_PIPE TOKEN_IDENTIFIER TOKEN_FLAG TOKEN_EOF

# Redirection tokens (shell)

- > => TOKEN_REDIR_OUT
- > > => TOKEN_REDIR_APPEND
- < => TOKEN_REDIR_IN
- << => TOKEN_REDIR_HEREDOC
- <<- => TOKEN_REDIR_HEREDOC_STRIP
- <& => TOKEN_REDIR_FD_IN
- > & => TOKEN_REDIR_FD_OUT
- <> => TOKEN_REDIR_INOUT
- > | => TOKEN_REDIR_CLOBBER

Note: TOKEN_ASS_SHIFT is for the arithmetic operator >>=. It should not be used for > in shell redirections. If older tests still expect TOKEN_ASS_SHIFT for >, replace with TOKEN_REDIR_OUT.

Example one-liner to fix older tests (run from project root if needed):
sed -i 's/TOKEN_ASS_SHIFT/TOKEN_REDIR_OUT/g' srcs/test/lexer_tests.txt

IDs:

- You can prefix a test with an ID to make it addressable:
  [ECHO1] echo "x" => TOKEN_IDENTIFIER TOKEN_DOUBLE_QUOTED_STRING TOKEN_EOF
- If no ID is provided, an auto ID L<lineno> is assigned (e.g., L42).

Filtering:

- Run a single test by ID:
  make test-lexer-verbose ID=L42
  make test-lexer-verbose ID=ECHO1
- Filter by substring in input:
  make test-lexer-verbose GREP="echo x"

Verbose logging:

- Write detailed logs (raw output, parsed block, expected/actual) to a file:
  make test-lexer-verbose LOG=srcs/test/lexer_tests.log
- Or call directly:
  python3 srcs/test/run_lexer_tests.py --verbose --log-file srcs/test/lexer_tests.log

# Lexer tests: prettier console, clean logs

This repo includes a small helper `color_tee.py` that:

- runs your existing lexer test command,
- prints a color-enhanced, easier-to-read console output,
- writes a plain (no ANSI escapes) log file to `srcs/test/lexer_tests.log`.

Usage examples:

- With a Makefile target you already have (raw test command):

  - `make test-lexer-raw` (or whatever you currently run)

- Wrap it with the color tee:
  - `python3 srcs/test/color_tee.py -o srcs/test/lexer_tests.log -- make test-lexer-raw`

Direct command example:

- `python3 srcs/test/color_tee.py -- ./your_lexer_tests_runner.sh`

Options:

- `-o, --output-log`: path for the plain log (default: srcs/test/lexer_tests.log)
- `--no-color`: disable console colors

Notes:

- The console shows brighter labels (INPUT, EXPECTED, ACTUAL, DETAILS) and highlights obvious error phrases.
- The log file is deliberately kept format-free (no ANSI escapes), per your requirement.

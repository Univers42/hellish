#ifndef LEXER_H
#define LEXER_H

#include "libft.h"

typedef enum e_category_token
{
	CAT_SYS_OP = 0,			   // System Operators (|, &&, ;, etc.)
	CAT_ARIT_OP = 100,		   // Arithmetic Operators (+, *, <=, ++, etc.)
	CAT_REDIR_IO = 200,		   // I/O Redirections (<, >, >>, etc.)
	CAT_GROUPING = 300,		   // Grouping and Sub-shells ((), {}, etc.)
	CAT_KEYWORDS = 400,		   // Shell Keywords (if, while, function, etc.)
	CAT_LITERALS = 500,		   // String/Word/Number Literals
	CAT_WORD_LIST = 550,	   //
	CAT_INHIBITORS = 600,	   // Quoting and Inhibitors (', ", \)
	CAT_PARAM_EXP = 700,	   // Parameter Expansions (${...})
	CAT_PROCESS_SUBST = 800,   // Process Substitution (<(), >())
	CAT_GLOBBING = 900,		   // Pattern Matching/Globbing (*, ?, [], !)
	CAT_HISTORY_EXP = 1000,	   // History Expansions (!!, !word, etc.)
	CAT_SPECIAL_PARAMS = 1100, // Special Parameters ($#, $?, $!, etc.)  <- NEW CATEGORY
	CAT_SPECIAL_CHARS = 1200,  // Tilde, Hash, Bang (#, ~, !, general)
	CAT_CONTROL = 1300,		   // Control flow/meta (WHITESPACE, EOF, ERR)
	CAT_END_SENTINEL = 1400	   // Sentinel value for range checks
} t_category_token;

typedef enum e_token_type
{
	/* === 0-99: CAT_SYS_OP - System Operators & Command Separators === */
	TOKEN_OPERATOR = CAT_SYS_OP,
	TOKEN_PIPE,		 // |    - Pipeline
	TOKEN_PIPE_PIPE, // ||   - Logical OR (Command)
	TOKEN_AMPERSAND, // &    - Background Job
	TOKEN_AMP_AMP,	 // &&   - Logical AND (Command)
	TOKEN_SEMICOLON, // ;    - Command separator
	TOKEN_NEWLINE,	 // '\n' - Command terminator

	/* === 100-199: CAT_ARIT_OP - Arithmetic Operators (for $((...)) or `let`) === */
	TOKEN_ARITH_OP = CAT_ARIT_OP,
	// Increment/Decrement
	TOKEN_OP_INCREMENT, // ++   - Combined (Lexer provides this, parser resolves pre/post)
	TOKEN_OP_DECREMENT, // --   - Combined
	// Unary/Additive/Multiplicative
	TOKEN_OP_ADD,	// +    - Addition/Unary Plus
	TOKEN_OP_MINUS, // -    - Subtraction/Unary Minus
	TOKEN_OP_MULT,	// * - Multiplication
	TOKEN_OP_DIV,	// /    - Division
	TOKEN_OP_MOD,	// %    - Modulo
	// Comparison
	TOKEN_COMP_EQ,	 // ==   - Equality
	TOKEN_COMP_NE,	 // !=   - Inequality
	TOKEN_COMP_GT,	 // >    - Greater Than
	TOKEN_COMP_LT,	 // <    - Less Than
	TOKEN_COMP_GTEQ, // >=   - Greater Than or Equal
	TOKEN_COMP_LTEQ, // <=   - Less Than or Equal
	// Logical
	TOKEN_LOGICAL_AND,	 // &&   - Logical AND (Arithmetic)
	TOKEN_LOGICAL_OR,	 // ||   - Logical OR (Arithmetic)
	TOKEN_TERNARY_COND,	 // ?    - Ternary conditional operator component (expr ? expr : expr)
	TOKEN_TERNARY_COLON, // :    - Ternary conditional operator component
	TOKEN_SIZEOF,		 // sizeof - C-style operator (if supported)
	// Bitwise and Assignment Operators
	TOKEN_OP_SHIFT_LEFT,  // <<
	TOKEN_OP_SHIFT_RIGHT, // >>
	TOKEN_OP_BITWISE_AND, // &
	TOKEN_OP_BITWISE_OR,  // |
	TOKEN_OP_BITWISE_XOR, // ^
	// assignment
	TOKEN_ASS_EQUAL,   // =    - Assignment
	TOKEN_ASS_PLUS,	   // +=
	TOKEN_ASS_MULT,	   // *=	- multi assignment
	TOKEN_ASS_DIV,	   // /=
	TOKEN_ASS_MOD,	   // %=
	TOKEN_ASS_MINUS,   // -=
	TOKEN_ASS_OR,	   // |=
	TOKEN_ASSS_AND,	   // &=
	TOKEN_ASS_SHIFT,   // >>=
	TOKEN_ASS_UNSHIFT, // <<=
	TOKEN_ASS_XOR,	   // ^=

	/* === 200-299: CAT_REDIR_IO - I/O Redirections === */
	// These tokens are used by the shell grammar. Arithmetic operators like >>=
	// remain under CAT_ARIT_OP and must not be used for I/O redirections.
	TOKEN_REDIR_IO = CAT_REDIR_IO,
	TOKEN_REDIR_IN,			   // <
	TOKEN_REDIR_OUT,		   // >
	TOKEN_REDIR_APPEND,		   // >>
	TOKEN_REDIR_HEREDOC,	   // <<
	TOKEN_REDIR_HEREDOC_STRIP, // <<-
	TOKEN_REDIR_FD_IN,		   // <&
	TOKEN_REDIR_FD_OUT,		   // >&
	TOKEN_REDIR_INOUT,		   // <>
	TOKEN_REDIR_CLOBBER,	   // >|

	/* === 300-399: CAT_GROUPING - Grouping & Sub-shells === */
	TOKEN_GROUPING = CAT_GROUPING,
	TOKEN_LEFT_PAREN,  // (    - Sub-shell start / Grouping start
	TOKEN_RIGHT_PAREN, // )    - Sub-shell end / Grouping end
	TOKEN_LEFT_BRACE,  // {    - Command group start
	TOKEN_RIGHT_BRACE, // }    - Command group end
	// Command and Arithmetic Substitution starters belong here as they define groups
	TOKEN_CMD_SUBST_START, // $(   - Control substitution (was TOKEN_DOLLAR_LPAREN)
	TOKEN_ARITH_EXP_START, // $((  - Arithmetic expansion (was TOKEN_DOLLAR_DPAREN)

	/* === 400-499: CAT_KEYWORDS - Shell Keywords - Reserved words === */
	TOKEN_KEYWORDS = CAT_KEYWORDS,
	TOKEN_IF,		// if
	TOKEN_THEN,		// then
	TOKEN_ELSE,		// else
	TOKEN_ELIF,		// elif
	TOKEN_FI,		// fi
	TOKEN_CASE,		// case
	TOKEN_ESAC,		// esac
	TOKEN_FOR,		// for
	TOKEN_SELECT,	// select
	TOKEN_WHILE,	// while
	TOKEN_UNTIL,	// until
	TOKEN_DO,		// do
	TOKEN_DONE,		// done
	TOKEN_FUNCTION, // function (or fun/FUN)
	TOKEN_COPROC,
	TOKEN_COND_START,
	TOKEN_COND_END,
	TOKEN_COND_ERROR,
	TOKEN_IN,
	TOKEN_THIS,
	TOKEN_BANG,
	TOKEN_TIME,
	TOKEN_TIMEOPT,
	TOKEN_TIMEIGN,
	// Retained custom keywords:

	/* === 500-599: CAT_LITERALS - Words and Literals === */
	TOKEN_LITERALS = CAT_LITERALS,
	TOKEN_IDENTIFIER, // Variable/function name identifier
	TOKEN_WORD,		  // General unquoted word
	TOKEN_ASS_WORD,
	TOKEN_REDIR_WORD,
	TOKEN_FLAG,	  // -flag / --long-flag (command-line style)
	TOKEN_NUMBER, // Numeric literal
	// The following tokens should represent the *content* or *type* if the lexer fully processes them:
	TOKEN_SINGLE_QUOTED_STRING, // '...' content
	TOKEN_DOUBLE_QUOTED_STRING, // "..." content
	TOKEN_BQUOTE,

	TOKEN_LISTS = CAT_WORD_LIST,
	TOKEN_ARITH_CMD,
	TOKEN_ARITH_FOR_EXPRS,

	/* === 600-699: CAT_INHIBITORS - Quoting and Inhibitors === */
	TOKEN_INHIBITORS = CAT_INHIBITORS,
	TOKEN_BACKSLASH,	// \    - Inhibitor (backslash)
	TOKEN_DQUOTE_START, // "    - Inhibitor start
	TOKEN_SQUOTE_START, // '    - Inhibitor start
	TOKEN_BQUOTE_START, // `    - Command substitution start (old style)
	TOKEN_DQUOTE_END,
	TOKEN_BQUOTE_END,
	TOKEN_SQUOTE_END,
	/* === 700-799: CAT_PARAM_EXP - Parameter Expansion (${...}) === */
	TOKEN_PAR_FORMAT = CAT_PARAM_EXP,
	TOKEN_PAR_EXP_START, // ${   - Parameter expansion start (was TOKEN_DOLLAR_BRACE)
	// Parameter format operators:
	TOKEN_COLON_DASH,	  // :-
	TOKEN_COLON_EQUAL,	  // :=
	TOKEN_COLON_QUESTION, // :?
	TOKEN_COLON_PLUS,	  // :+
	// Substring/Length/Glob operators:
	TOKEN_HASH_OP,		   // #    - For ${#parameter} (length)
	TOKEN_PERCENT,		   // %    - For ${parameter%pattern}
	TOKEN_PERCENT_PERCENT, // %%
	TOKEN_POUND,		   // #    - For ${parameter#pattern} (Smallest prefix removal)
	TOKEN_POUND_POUND,	   // ##   - For ${parameter##pattern} (Largest prefix removal)
	// End brace is a general grouping token, but keep the start here for category clarity.

	/* === 800-899: CAT_PROCESS_SUBST - Process Substitution === */
	TOKEN_PROCESS_SUBST = CAT_PROCESS_SUBST,
	TOKEN_SUBST_IN,	 // <(   - Process Substitution (input)
	TOKEN_SUBST_OUT, // >(   - Process Substitution (output)

	/* === 900-999: CAT_GLOBBING - Pattern Matching (Globbing) === */
	TOKEN_GLOBBING = CAT_GLOBBING,
	TOKEN_GLOB_STAR,		  // * - Zero or more characters
	TOKEN_GLOB_QUESTION,	  // ?    - Single character
	TOKEN_GLOB_BRACKET_OPEN,  // [    - Character class start
	TOKEN_GLOB_BRACKET_CLOSE, // ]    - Character class end
	TOKEN_GLOB_NEGATION,	  // !    - Negation inside character class ([!chars])

	/* === 1000-1099: CAT_HISTORY_EXP - History Expansion === */
	TOKEN_HIST_EXP = CAT_HISTORY_EXP,
	TOKEN_BANG_BANG,		// !!   - Previous command
	TOKEN_BANG_WORD,		// !word- Event designator
	TOKEN_BANG_NUMBER,		// !n
	TOKEN_BANG_DASH_NUMBER, // !-n

	/* === 1100-1199: CAT_SPECIAL_PARAMS - Special Parameters === */
	TOKEN_SPECIAL_PARAMS = CAT_SPECIAL_PARAMS,
	TOKEN_DOLLAR_HASH,	   // $#   - Argument count
	TOKEN_DOLLAR_STAR,	   // $* - All positional parameters
	TOKEN_DOLLAR_AT,	   // $@   - All positional parameters (quoted)
	TOKEN_DOLLAR_QUESTION, // $?   - Last exit status
	TOKEN_DOLLAR_DOLLAR,   // $$   - Current shell pid
	TOKEN_DOLLAR_BANG,	   // $!   - Last background job PID
	TOKEN_DOLLAR_DASH,	   // $-   - Current shell flags
	TOKEN_DOLLAR_ZERO,	   // $0   - Shell/script name
	TOKEN_AT,			   // @    - Positional parameter access (@ for $1, $2, ...)
	TOKEN_QUESTION,		   // ?    - (Standalone) Used for globbing/ternary, but keep $? here.

	/* === 1200-1299: CAT_SPECIAL_CHARS - Other Special Characters === */
	TOKEN_SPECIAL_CHARS = CAT_SPECIAL_CHARS,
	TOKEN_TILDE, // ~    - Tilde expansion (standalone)
	TOKEN_HASH,	 // #    - Comment start (standalone)
	// TOKEN_BANG,                 // !    - Negation/History/Globbing prefix (standalone)
	TOKEN_DOLLAR, // $    - Variable prefix (standalone, e.g., $var)
	TOKEN_DASH,	  // -    - Previous directory / Option delimiter (standalone)
	TOKEN_COMMA,  // ,
	TOKEN_DOT,	  // .
	/* === 1300-1399: CAT_CONTROL - Control/Metadata Tokens === */
	TOKEN_CONTROL = CAT_CONTROL,
	TOKEN_WHITESPACE, // Space/tab (usually ignored)
	TOKEN_EOF,		  // End of file
	TOKEN_ERR,		  // Lexer Error
	TOKEN_COUNT,	  // Total number of token types
	TOKEN_SENTINEL
} t_token_type;

typedef enum e_stream_type
{
	STREAM_NONE,
	STREAM_STRING,
	STREAM_FILE,
	STREAM_STDIN,
} t_stream_type;

typedef struct s_map_token
{
	t_token_type token;
	const char *rep_name;
} t_map_token;

extern t_map_token tok[];
// --- 1. Scanner Structure ---
// No globals. This struct will be passed around.

// --- 2. Token Handler Function Type ---

// --- 3. Singleton Accessor ---
// This is the *only* function you need to call to get the lookup tables.

// --- 4. The Singleton's Data ---
// This struct is what the singleton function returns.

typedef struct s_token
{
	t_token_type type;
	const char *start;
	int length;
	int line;
} t_token;

typedef union u_input_source
{
	char *string;
	void *file;
} t_input_source;

typedef struct s_input_stream
{
	t_stream_type type;
	t_input_source src;
	size_t idx;
	size_t len;
	char *name;
} t_input_stream;

typedef struct s_delim_stack
{
	const char *delimiters;
	int depth;
	int capacity;
} t_delim_stack;

typedef struct s_reserved_word
{
	const char *word;
	t_token_type token;
} t_reserved_word;

typedef struct s_keyword_entry
{
	const char *keyword;
	int length;
	t_token_type type;
} t_keyword_entry;

typedef struct s_scanner
{
	t_input_stream input;
	t_delim_stack dstack;
	const char *start;
	const char *current;
	size_t token_index;
	size_t token_cap;
	int lexer_state;
	int last_token;
	int current_char;
	int interactive;
	int expand_aliases;
	int line;
} t_scanner;

t_token scan_token(t_scanner *scan);
typedef t_token (*t_fmt)(t_scanner *scan);
typedef struct s_dispatch_data
{
	uint64_t fast_path_map;
	t_fmt dispatch_table[128];
} t_dispatch_data;

static inline const t_keyword_entry *map_keywords(void)
{
	static const t_keyword_entry keywords[] = {
		{"if", 2, TOKEN_IF},
		{"else", 4, TOKEN_ELSE},
		{"while", 5, TOKEN_WHILE},
		{"for", 3, TOKEN_FOR},
		{"do", 2, TOKEN_DO},
		{"done", 4, TOKEN_DONE},
		{NULL, 0, TOKEN_SENTINEL}};
	return keywords;
}

static inline int keyword_hash(const char *str, int length)
{
	return (str[0] + str[length - 1 + length] % 32);
}

const t_dispatch_data *get_dispatch_singleton(void);
char advance(t_scanner *scan);
char peek(t_scanner *scan);
bool scan_is_at_end(t_scanner *scan);
char peek_next(t_scanner *scan);
bool match(t_scanner *scan, char expected);
t_token make_token(t_scanner *scan, t_token_type type);
t_token error_token(t_scanner *scan, const char *mesg);
void skip_unused_data(t_scanner *scan);
t_token_type check_key_word(int start, int length,
							const char *rest, t_token_type type, t_scanner *scan);
t_token_type identifier_type(t_scanner *scan);
t_token identifier(t_scanner *scan);
t_token number(t_scanner *scan);
t_token string(t_scanner *scan);
t_token scan_token(t_scanner *scan);
void print_token(const t_token *token);
void scan_all_tokens(t_scanner *scan, int debug);
bool is_redir_token(t_token_type type);
bool is_logical_token(t_token_type type);
bool is_keyword_token(t_token_type type);
bool is_quote_token(t_token_type type);
bool is_expansion(t_token_type type);

typedef t_token (*t_char_handler)(t_scanner *scanner);

/* Character handler functions */
t_token handle_pipe(t_scanner *scanner);
t_token handle_ampersand(t_scanner *scanner);
t_token handle_semicolon(t_scanner *scanner);
t_token handle_greater(t_scanner *scanner);
t_token handle_less(t_scanner *scanner);
t_token handle_lparen(t_scanner *scanner);
t_token handle_rparen(t_scanner *scanner);
t_token handle_lbrace(t_scanner *scanner);
t_token handle_rbrace(t_scanner *scanner);
t_token handle_newline(t_scanner *scanner);
t_token handle_squote(t_scanner *scanner);
t_token handle_dquote(t_scanner *scanner);
t_token handle_dollar(t_scanner *scanner);
t_token handle_bquote(t_scanner *scanner);
t_token handle_backslash(t_scanner *scanner);
t_token handle_hash(t_scanner *scanner);
t_token handle_tilde(t_scanner *scanner);
t_token handle_star(t_scanner *scanner);
t_token handle_percent(t_scanner *scanner); // ensure declared
t_token handle_question(t_scanner *scanner);
t_token handle_lbracket(t_scanner *scanner);
t_token handle_rbracket(t_scanner *scanner);
t_token handle_dot(t_scanner *scanner);
t_token handle_word(t_scanner *scanner);
t_token handle_whitespace(t_scanner *scanner);
t_token handle_eof(t_scanner *scanner);
t_token handle_error(t_scanner *scanner);
t_token handle_colon(t_scanner *scanner); // new

// --- 5. Handler Prototypes ---
t_token handle_glob(t_scanner *scan);
t_token handle_bang(t_scanner *scan);
t_token handle_tilde(t_scanner *scan);
t_token handle_hash(t_scanner *scan);
t_token handle_whitespace(t_scanner *scan);
t_token handle_plus(t_scanner *scan);
t_token handle_div(t_scanner *scan);
t_token handle_percent(t_scanner *scan);
t_token handle_eq(t_scanner *scan);
t_token handle_glob(t_scanner *scan);
t_token handle_minus(t_scanner *scan);
t_token handle_sys_op(t_scanner *scan);
t_token handle_literal(t_scanner *scan);
t_scanner *init_scanner(const char *source);
#endif

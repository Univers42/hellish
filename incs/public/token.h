#ifndef TOKEN_H
# define TOKEN_H

# include <stdbool.h>

/* token types used by lexer/parser */
typedef enum e_tt
{
	TT_WORD,
	TT_SQWORD,
	TT_DQWORD,
	TT_DQENVVAR,
	TT_ENVVAR,
	TT_HEREDOC,
	TT_REDIRECT_LEFT,
	TT_REDIRECT_RIGHT,
	TT_APPEND,
	TT_PIPE,
	TT_BRACE_LEFT,
	TT_BRACE_RIGHT,
	TT_AND,
	TT_OR,
	TT_SEMICOLON,
	TT_NEWLINE,
	TT_END,
	TT_AMPERSAND,  /* single & for background execution */
} t_tt;

/* compact representation of original full token for AST bookkeeping */
typedef struct s_token_old
{
	char	*start;
	int		len;
	bool	present;
} t_token_old;

/* token struct used in AST and lexer */
typedef struct s_token
{
	char		*start;
	int			len;
	t_tt		tt;
	bool		allocated; /* whether start was heap-allocated */
	t_token_old	full_word; /* optional saved original token */
} t_token;

#endif
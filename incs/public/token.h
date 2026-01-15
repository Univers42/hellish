#ifndef TOKEN_H
# define TOKEN_H

# include "libft.h"

/* token types used by lexer/parser */
typedef enum e_tt
{
	TT_END,
	TT_WORD,
	TT_REDIRECT_LEFT,
	TT_REDIRECT_RIGHT,
	TT_APPEND,
	TT_PIPE,
	TT_BRACE_LEFT,
	TT_BRACE_RIGHT,
	TT_OR,
	TT_AND,
	TT_SEMICOLON,
	TT_HEREDOC,
	TT_NEWLINE,
	TT_SQWORD,
	TT_DQWORD,
	TT_ENVVAR,
	TT_DQENVVAR,
	TT_AMPERSAND,
	TT_ARITH_START,
	TT_PROC_SUB_IN,   /* <( process substitution input */
	TT_PROC_SUB_OUT   /* >( process substitution output */
}	t_tt;

/* compact representation of original full token for AST bookkeeping */
typedef struct s_token_old
{
	bool	present;
	char	*start;
	int		len;
}	t_token_old;

/* token struct used in AST and lexer */
typedef struct s_token
{
	t_tt		tt;
	char		*start;
	int			len;
	bool		allocated;
	t_token_old	full_word;
}	t_token;

#endif
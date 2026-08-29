/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:46:57 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARITH_H
# define ARITH_H

# include <stdbool.h>
# include <stdint.h>

/* Forward declaration */
typedef struct s_shell	t_shell;

/* Arithmetic token types */
typedef enum e_arith_tok
{
	ATOK_NUM,		/* number literal */
	ATOK_VAR,		/* variable name */
	ATOK_PLUS,		/* + */
	ATOK_MINUS,		/* - */
	ATOK_MUL,		/* * */
	ATOK_DIV,		/* / */
	ATOK_MOD,		/* % */
	ATOK_POW,		/* ** (exponentiation) */
	ATOK_LPAREN,	/* ( */
	ATOK_RPAREN,	/* ) */
	ATOK_EQ,		/* == */
	ATOK_NE,		/* != */
	ATOK_LT,		/* < */
	ATOK_LE,		/* <= */
	ATOK_GT,		/* > */
	ATOK_GE,		/* >= */
	ATOK_AND,		/* && */
	ATOK_OR,		/* || */
	ATOK_NOT,		/* ! */
	ATOK_BAND,		/* & (bitwise) */
	ATOK_BOR,		/* | (bitwise) */
	ATOK_BXOR,		/* ^ (bitwise) */
	ATOK_BNOT,		/* ~ (bitwise not) */
	ATOK_LSHIFT,	/* << */
	ATOK_RSHIFT,	/* >> */
	ATOK_ASSIGN,	/* = */
	ATOK_INC,		/* ++ */
	ATOK_DEC,		/* -- */
	ATOK_TERNQ,		/* ? (ternary) */
	ATOK_TERNC,		/* : (ternary) */
	ATOK_COMMA,		/* , */
	ATOK_EOF,		/* end of expression */
	ATOK_ERROR,		/* error token */
}	t_arith_tok;

/* Token structure for arithmetic lexer */
typedef struct s_arith_token
{
	t_arith_tok	type;
	long long	num_val;	/* value if ATOK_NUM */
	char		*var_name;	/* variable name if ATOK_VAR (not allocated) */
	int			var_len;	/* length of variable name */
	const char	*start;		/* pointer to start in source */
	int			len;		/* length of token */
}	t_arith_token;

/* A pure $((...)) expression lexed once: the token array (with var_name/start
   pointing into the persistent source bytes) plus the source identity so the
   cache is only reused for the same bytes. Variables still resolve live. */
typedef struct s_arith_cache
{
	t_arith_token	*toks;
	int				ntoks;
	const char		*src;
	int				srclen;
}	t_arith_cache;

/* Lexer state. When `toks` is set the lexer replays that cached array (using
   `pos` as the token index, so the parser's pos-based backtracking rewinds it
   transparently); otherwise it scans `input` with `pos` as the byte offset. */
typedef struct s_arith_lexer
{
	const char			*input;
	int					pos;
	int					len;
	t_arith_token		current;
	bool				error;
	char				*error_msg;
	const t_arith_token	*toks;
	int					ntoks;
}	t_arith_lexer;

/* Parser/evaluator state */
typedef struct s_arith_parser
{
	t_arith_lexer	*lexer;
	t_shell			*shell;
	bool			error;
	bool			no_side_effects;
	char			*error_msg;
}	t_arith_parser;

char			*zsh_param(t_shell *state, const char *s, int slen);
bool			lex_zsh_plus(t_arith_lexer *lex);

/* Main API */
char			*arith_expand(t_shell *state, const char *expr, int len);
long long		arith_eval(t_shell *state,
					const char *expr, int len, bool *error);

/* Cached arithmetic: lex once into *cachep, then re-evaluate live. */
char			*arith_expand_cached(t_shell *state, const char *expr,
					int len, t_arith_cache **cachep);
void			arith_cache_free(t_arith_cache *c);

/* Lexer functions */
void			arith_lexer_init(t_arith_lexer *lex,
					const char *input, int len);
void			arith_lexer_init_toks(t_arith_lexer *lex,
					const t_arith_token *toks, int ntoks);
void			arith_lexer_advance(t_arith_lexer *lex);
t_arith_token	arith_lexer_peek(t_arith_lexer *lex);

/* Parser functions (recursive descent) */
long long		arith_parse_expr(t_arith_parser *p);
long long		arith_parse_ternary(t_arith_parser *p);
long long		arith_parse_or(t_arith_parser *p);
long long		arith_parse_and(t_arith_parser *p);
long long		arith_parse_bitor(t_arith_parser *p);
long long		arith_parse_bitxor(t_arith_parser *p);
long long		arith_parse_bitand(t_arith_parser *p);
long long		arith_parse_equality(t_arith_parser *p);
long long		arith_parse_relational(t_arith_parser *p);
long long		arith_parse_shift(t_arith_parser *p);
long long		arith_parse_additive(t_arith_parser *p);
long long		arith_parse_multiplicative(t_arith_parser *p);
long long		arith_parse_unary(t_arith_parser *p);
long long		arith_parse_exponent(t_arith_parser *p);
long long		arith_parse_primary(t_arith_parser *p);

#endif

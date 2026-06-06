/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith_private.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:13:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARITH_PRIVATE_H
# define ARITH_PRIVATE_H

# include "arith.h"
# include "shell.h"
# include "libft.h"
# include <stdlib.h>
# include <limits.h>
# include "helpers.h"
# include "env.h"

long long		arith_eval(t_shell *state, const char *expr,
					int len, bool *error);
char			*arith_expand(t_shell *state, const char *expr, int len);

/* Shared eval helpers (eval.c), reused by the cached path (eval_cached.c).
   arith_run is the single parse+evaluate kernel; the two public entry points
   (arith_eval and arith_eval_cached) differ only in how they initialize the
   lexer before handing it to arith_run. */
long long		arith_run(t_shell *state, t_arith_lexer *lexer, bool *error);
char			*arith_lltoa(long long value);
char			*arith_fail(t_shell *state, const char *expr, int len);
long long		arith_eval_cached(t_shell *state, t_arith_cache *c,
					bool *error);
void			arith_advance_toks(t_arith_lexer *lex);

/* Lexer internals -- used only within src/arith/ */
void			skip_whitespace(t_arith_lexer *lex);
bool			is_var_start(char c);
bool			is_var_char(char c);
void			lex_number(t_arith_lexer *lex);
void			lex_variable(t_arith_lexer *lex);
void			lex_dollar_var(t_arith_lexer *lex);
void			lex_two_char_op(t_arith_lexer *lex, char c2,
					t_arith_tok single, t_arith_tok dbl);
void			lex_operator(t_arith_lexer *lex);
void			arith_lexer_init(t_arith_lexer *lex,
					const char *input, int len);
void			arith_lexer_advance(t_arith_lexer *lex);
t_arith_token	arith_lexer_peek(t_arith_lexer *lex);

long long		get_var_value(t_arith_parser *p, const char *name, int len);
void			set_var_value(t_arith_parser *p, const char *name, int len,
					long long val);
void			expect(t_arith_parser *p, t_arith_tok type);
bool			is_compound_op(t_arith_tok op);
long long		apply_op(long long l, long long r,
					t_arith_tok op, t_arith_parser *p);
long long		try_compound_assign(t_arith_parser *p,
					t_arith_token *var, long long val);
long long		arith_parse_unary(t_arith_parser *p);
long long		arith_parse_primary(t_arith_parser *p);
long long		arith_parse_exponent(t_arith_parser *p);
long long		arith_parse_multiplicative(t_arith_parser *p);
long long		arith_parse_additive(t_arith_parser *p);
long long		arith_parse_shift(t_arith_parser *p);
long long		arith_parse_relational(t_arith_parser *p);
long long		arith_parse_equality(t_arith_parser *p);

/* Bitwise and logical level parsers (parser2.c, helpers6.c, helpers11.c) */
long long		arith_parse_bitand(t_arith_parser *p);
long long		arith_parse_bitxor(t_arith_parser *p);
long long		arith_parse_bitor(t_arith_parser *p);
long long		arith_parse_and(t_arith_parser *p);
long long		arith_parse_or(t_arith_parser *p);
long long		arith_parse_ternary(t_arith_parser *p);
long long		arith_parse_expr(t_arith_parser *p);

/* Precedence climber (arith_climb.c / arith_climb2.c): collapses the
   bitor..multiplicative range into one loop instead of ~8 nested functions. */
long long		apply_binop(t_arith_parser *p, t_arith_tok type,
					long long l, long long r);
long long		arith_parse_binop(t_arith_parser *p, int min_prec);

void			handle_angle_right(t_arith_lexer *lex);
long long		parse_digits(const char *input, int *pos,
					int len, int base);
long long		parse_base_n(t_arith_lexer *lex, int *pos,
					long long base);

/* Lexer token-emit helpers (set.c) */
void			set_simple_op(t_arith_lexer *lex, int type);
void			set_lex_error(t_arith_lexer *lex);

#endif
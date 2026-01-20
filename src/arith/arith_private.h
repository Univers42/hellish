/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith_private.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:13:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:20:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARITH_PRIVATE_H
# define ARITH_PRIVATE_H

# include "arith.h"
# include "shell.h"
# include "libft.h"
# include <stdlib.h>
# include "helpers.h"
# include "env.h"

long long		arith_eval(t_shell *state, const char *expr,
					int len, bool *error);
char			*arith_expand(t_shell *state, const char *expr, int len);

//lexer
void			skip_whitespace(t_arith_lexer *lex);
bool			is_var_start(char c);
bool			is_var_char(char c);
void			lex_number(t_arith_lexer *lex);
void			lex_variable(t_arith_lexer *lex);
void			lex_two_char_op(t_arith_lexer *lex, char c2,
					t_arith_tok single, t_arith_tok dbl);
void			lex_operator(t_arith_lexer *lex);
void			arith_lexer_init(t_arith_lexer *lex,
					const char *input, int len);
void			arith_lexer_advance(t_arith_lexer *lex);
t_arith_token	arith_lexer_peek(t_arith_lexer *lex);

long long		get_var_value(t_arith_parser *p, const char *name, int len);
void			expect(t_arith_parser *p, t_arith_tok type);
long long		arith_parse_exponent(t_arith_parser *p);
long long		arith_parse_multiplicative(t_arith_parser *p);
long long		arith_parse_additive(t_arith_parser *p);
long long		arith_parse_shift(t_arith_parser *p);
long long		arith_parse_relational(t_arith_parser *p);
long long		arith_parse_equality(t_arith_parser *p);

//parser2.c
long long		arith_parse_bitand(t_arith_parser *p);
long long		arith_parse_bitxor(t_arith_parser *p);
long long		arith_parse_bitor(t_arith_parser *p);
long long		arith_parse_and(t_arith_parser *p);
long long		arith_parse_or(t_arith_parser *p);
long long		arith_parse_ternary(t_arith_parser *p);
long long		arith_parse_expr(t_arith_parser *p);

// set.c
void			set_simple_op(t_arith_lexer *lex, int type);
void			set_lex_error(t_arith_lexer *lex);

#endif
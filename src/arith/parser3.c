/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 04:13:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

long long	try_compound_assign(t_arith_parser *p,
	t_arith_token *var, long long val)
{
	int				sp;
	t_arith_token	sc;
	t_arith_tok		op;

	op = arith_lexer_peek(p->lexer).type;
	if (!is_compound_op(op))
		return (val);
	sp = p->lexer->pos;
	sc = p->lexer->current;
	arith_lexer_advance(p->lexer);
	if (arith_lexer_peek(p->lexer).type != ATOK_ASSIGN)
	{
		p->lexer->pos = sp;
		p->lexer->current = sc;
		return (val);
	}
	arith_lexer_advance(p->lexer);
	val = apply_op(val, arith_parse_ternary(p), op, p);
	set_var_value(p, var->var_name, var->var_len, val);
	return (val);
}

static long long	handle_prefix_inc(t_arith_parser *p)
{
	t_arith_token	tok;
	long long		val;

	arith_lexer_advance(p->lexer);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_VAR)
		return (p->error = true, 0);
	arith_lexer_advance(p->lexer);
	val = get_var_value(p, tok.var_name, tok.var_len) + 1;
	set_var_value(p, tok.var_name, tok.var_len, val);
	return (val);
}

static long long	handle_prefix_dec(t_arith_parser *p)
{
	t_arith_token	tok;
	long long		val;

	arith_lexer_advance(p->lexer);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_VAR)
		return (p->error = true, 0);
	arith_lexer_advance(p->lexer);
	val = get_var_value(p, tok.var_name, tok.var_len) - 1;
	set_var_value(p, tok.var_name, tok.var_len, val);
	return (val);
}

/* Unary: ('++' | '--' | '+' | '-' | '!' | '~') unary | postfix */
long long	arith_parse_unary(t_arith_parser *p)
{
	t_arith_token	tok;

	if (p->error)
		return (0);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_INC)
		return (handle_prefix_inc(p));
	if (tok.type == ATOK_DEC)
		return (handle_prefix_dec(p));
	if (tok.type == ATOK_PLUS)
		return (arith_lexer_advance(p->lexer), arith_parse_unary(p));
	if (tok.type == ATOK_MINUS)
		return (arith_lexer_advance(p->lexer), -arith_parse_unary(p));
	if (tok.type == ATOK_NOT)
		return (arith_lexer_advance(p->lexer), !arith_parse_unary(p));
	if (tok.type == ATOK_BNOT)
		return (arith_lexer_advance(p->lexer), ~arith_parse_unary(p));
	return (arith_parse_primary(p));
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

static long long	parse_expr_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_expr(p);
	p->no_side_effects = old;
	return (val);
}

static long long	parse_ternary_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_ternary(p);
	p->no_side_effects = old;
	return (val);
}

/* Handle the true-branch of a ternary when cond != 0 */
static long long	ternary_true(t_arith_parser *p)
{
	long long		then_val;
	t_arith_token	tok;

	then_val = arith_parse_expr(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNC)
	{
		p->error = true;
		return (0);
	}
	arith_lexer_advance(p->lexer);
	(void)parse_ternary_nse(p);
	return (then_val);
}

/* Ternary: or ('?' expr ':' ternary)? */
long long	arith_parse_ternary(t_arith_parser *p)
{
	long long		cond;
	long long		else_val;
	t_arith_token	tok;

	cond = arith_parse_or(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNQ)
		return (cond);
	arith_lexer_advance(p->lexer);
	if (cond != 0)
		return (ternary_true(p));
	(void)parse_expr_nse(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNC)
	{
		p->error = true;
		return (0);
	}
	arith_lexer_advance(p->lexer);
	else_val = arith_parse_ternary(p);
	return (else_val);
}

/* Expression: ternary (',' ternary)* - comma operator */
long long	arith_parse_expr(t_arith_parser *p)
{
	long long		val;
	t_arith_token	tok;

	val = arith_parse_ternary(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_COMMA)
		{
			arith_lexer_advance(p->lexer);
			val = arith_parse_ternary(p);
		}
		else
			break ;
	}
	return (val);
}

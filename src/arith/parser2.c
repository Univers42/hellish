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

static long long	parse_expr_no_side_effects(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_expr(p);
	p->no_side_effects = old;
	return (val);
}

static long long	parse_ternary_no_side_effects(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_ternary(p);
	p->no_side_effects = old;
	return (val);
}

/* Ternary: or ('?' expr ':' ternary)? */
long long	arith_parse_ternary(t_arith_parser *p)
{
	long long		cond;
	long long		then_val;
	long long		else_val;
	t_arith_token	tok;

	cond = arith_parse_or(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_TERNQ)
	{
		arith_lexer_advance(p->lexer);
		if (cond != 0)
		{
			then_val = arith_parse_expr(p);
			tok = arith_lexer_peek(p->lexer);
			if (tok.type != ATOK_TERNC)
			{
				p->error = true;
				return (0);
			}
			arith_lexer_advance(p->lexer);
			(void)parse_ternary_no_side_effects(p);
			return (then_val);
		}
		(void)parse_expr_no_side_effects(p);
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
	return (cond);
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

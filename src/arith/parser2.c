/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:20:02 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

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
		then_val = arith_parse_expr(p);
		tok = arith_lexer_peek(p->lexer);
		if (tok.type != ATOK_TERNC)
		{
			p->error = true;
			return (0);
		}
		arith_lexer_advance(p->lexer);
		else_val = arith_parse_ternary(p);
		if (cond)
			return (then_val);
		else
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

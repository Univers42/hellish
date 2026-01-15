/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith.h"
#include "libft.h"

/* Bitand: equality ('&' equality)* */
long long	arith_parse_bitand(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_equality(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BAND)
		{
			arith_lexer_advance(p->lexer);
			left = left & arith_parse_equality(p);
		}
		else
			break;
	}
	return (left);
}

/* Bitxor: bitand ('^' bitand)* */
long long	arith_parse_bitxor(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitand(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BXOR)
		{
			arith_lexer_advance(p->lexer);
			left = left ^ arith_parse_bitand(p);
		}
		else
			break;
	}
	return (left);
}

/* Bitor: bitxor ('|' bitxor)* */
long long	arith_parse_bitor(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitxor(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BOR)
		{
			arith_lexer_advance(p->lexer);
			left = left | arith_parse_bitxor(p);
		}
		else
			break;
	}
	return (left);
}

/* And: bitor ('&&' bitor)* */
long long	arith_parse_and(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitor(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_AND)
		{
			arith_lexer_advance(p->lexer);
			left = left && arith_parse_bitor(p);
		}
		else
			break;
	}
	return (left);
}

/* Or: and ('||' and)* */
long long	arith_parse_or(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_and(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_OR)
		{
			arith_lexer_advance(p->lexer);
			left = left || arith_parse_and(p);
		}
		else
			break;
	}
	return (left);
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
		then_val = arith_parse_expr(p);
		tok = arith_lexer_peek(p->lexer);
		if (tok.type != ATOK_TERNC)
		{
			p->error = true;
			return (0);
		}
		arith_lexer_advance(p->lexer);
		else_val = arith_parse_ternary(p);
		return (cond ? then_val : else_val);
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
			break;
	}
	return (val);
}

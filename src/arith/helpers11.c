/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers11.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:22:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Evaluate arith_parse_binop (bitor level, min_prec=1) with side effects
   suppressed -- used to silently consume the false side of && without firing
   any assignments or increments inside it. */
static long long	parse_bitor_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_binop(p, 1);
	p->no_side_effects = old;
	return (val);
}

/* Same as parse_bitor_nse but for a full && expression -- used by
   arith_parse_or to consume the dead right operand of a true ||
   without side effects. */
static long long	parse_and_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_and(p);
	p->no_side_effects = old;
	return (val);
}

/* Logical AND: bitor ('&&' bitor)*. Short-circuits: if left is already 0 the
   right operand is parsed with no_side_effects so we advance past it without
   triggering any variable writes. If left is non-zero the right operand is
   evaluated normally and the result is converted to 0/1. This is how bash
   handles `(( 0 && (x=5) ))` -- x stays unchanged. */
long long	arith_parse_and(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_binop(p, 1);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_AND)
		{
			arith_lexer_advance(p->lexer);
			if (left == 0)
				(void)parse_bitor_nse(p);
			else
				left = (arith_parse_binop(p, 1) != 0);
		}
		else
			break ;
	}
	return (left);
}

/* Logical OR: and ('||' and)*. Short-circuits: if left is non-zero the
   result is already 1 and the right operand is consumed with no_side_effects.
   If left is 0 the right operand is evaluated normally and its boolean value
   becomes the result. Mirrors the && short-circuit above but with the
   condition inverted. */
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
			if (left != 0)
			{
				left = 1;
				(void)parse_and_nse(p);
			}
			else
				left = (arith_parse_and(p) != 0);
		}
		else
			break ;
	}
	return (left);
}

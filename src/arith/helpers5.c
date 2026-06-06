/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers5.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:13:25 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Multiply left by the next exponent expression. No overflow guard -- C99
   signed overflow on long long is UB in theory, but POSIX shells traditionally
   wrap silently and that's what bash does. */
static long long	handle_mul(t_arith_parser *p, long long left)
{
	arith_lexer_advance(p->lexer);
	return (left * arith_parse_exponent(p));
}

/* Divide left by the next exponent expression. Two guards: divide-by-zero
   sets p->error unless we're in a dead ternary branch (no_side_effects), and
   LLONG_MIN / -1 would overflow -- bash returns LLONG_MIN in that corner,
   matching the kernel's signed-integer-overflow-wraps behaviour. */
static long long	handle_div(t_arith_parser *p, long long left)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_exponent(p);
	if (right == 0)
	{
		if (p->no_side_effects)
			return (0);
		p->error = true;
		return (0);
	}
	if (left == LLONG_MIN && right == -1)
		return (LLONG_MIN);
	return (left / right);
}

/* Modulo left by the next exponent expression. Same two guards as handle_div.
   For the LLONG_MIN % -1 case bash returns 0 (the mathematically correct
   result since -1 divides everything evenly), so we mirror that. */
static long long	handle_mod(t_arith_parser *p, long long left)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_exponent(p);
	if (right == 0)
	{
		if (p->no_side_effects)
			return (0);
		p->error = true;
		return (0);
	}
	if (left == LLONG_MIN && right == -1)
		return (0);
	return (left % right);
}

/* Quick check used as the loop guard in arith_parse_multiplicative. */
static bool	is_multiplicative_op(t_arith_tok type)
{
	return (type == ATOK_MUL || type == ATOK_DIV || type == ATOK_MOD);
}

/* Multiplicative: exponent (('*' | '/' | '%') exponent)*. Left-associative
   loop; each iteration dispatches to the appropriate handler so the UB guards
   are contained. Note arith_parse_binop (the climber) subsumes this level
   when called via arith_parse_and/or; this function is the direct fallback
   for code that builds the full descent chain manually. */
long long	arith_parse_multiplicative(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_exponent(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (!is_multiplicative_op(tok.type))
			break ;
		if (tok.type == ATOK_MUL)
			left = handle_mul(p, left);
		else if (tok.type == ATOK_DIV)
			left = handle_div(p, left);
		else if (tok.type == ATOK_MOD)
			left = handle_mod(p, left);
	}
	return (left);
}

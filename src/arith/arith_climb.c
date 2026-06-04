/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith_climb.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

static long long	apply_bitwise(t_arith_tok type, long long l, long long r)
{
	if (type == ATOK_BOR)
		return (l | r);
	if (type == ATOK_BXOR)
		return (l ^ r);
	return (l & r);
}

static long long	apply_cmp(t_arith_tok type, long long l, long long r)
{
	if (type == ATOK_EQ)
		return (l == r);
	if (type == ATOK_NE)
		return (l != r);
	if (type == ATOK_LT)
		return (l < r);
	if (type == ATOK_LE)
		return (l <= r);
	if (type == ATOK_GT)
		return (l > r);
	return (l >= r);
}

static long long	apply_shift_add(t_arith_tok type, long long l, long long r)
{
	if (type == ATOK_LSHIFT)
		return (l << r);
	if (type == ATOK_RSHIFT)
		return (l >> r);
	if (type == ATOK_PLUS)
		return (l + r);
	return (l - r);
}

/* Mul/div/mod, replicating helpers5.c exactly: /0 errors unless no_side_effects
   (then yields 0), and LLONG_MIN OP -1 is guarded against UB. */
static long long	apply_muldiv(t_arith_parser *p, t_arith_tok type,
				long long l, long long r)
{
	if (type == ATOK_MUL)
		return (l * r);
	if (r == 0)
	{
		if (!p->no_side_effects)
			p->error = true;
		return (0);
	}
	if (l == LLONG_MIN && r == -1)
	{
		if (type == ATOK_DIV)
			return (LLONG_MIN);
		return (0);
	}
	if (type == ATOK_DIV)
		return (l / r);
	return (l % r);
}

long long	apply_binop(t_arith_parser *p, t_arith_tok type,
			long long l, long long r)
{
	if (type == ATOK_MUL || type == ATOK_DIV || type == ATOK_MOD)
		return (apply_muldiv(p, type, l, r));
	if (type == ATOK_LSHIFT || type == ATOK_RSHIFT
		|| type == ATOK_PLUS || type == ATOK_MINUS)
		return (apply_shift_add(type, l, r));
	if (type == ATOK_BOR || type == ATOK_BXOR || type == ATOK_BAND)
		return (apply_bitwise(type, l, r));
	return (apply_cmp(type, l, r));
}

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

/* Apply one of the three bitwise binary operators. No UB cases here --
   bitwise ops on long long are well-defined for any bit pattern, so this
   is just three straight C operators grouped to keep apply_binop compact. */
static long long	apply_bitwise(t_arith_tok type, long long l, long long r)
{
	if (type == ATOK_BOR)
		return (l | r);
	if (type == ATOK_BXOR)
		return (l ^ r);
	return (l & r);
}

/* Apply a comparison operator; all six return 0 or 1 like in C. Signed
   comparison on long long is fine for POSIX shell integers. The final
   return (l >= r) is ATOK_GE -- reached only when all others have been
   ruled out by the chain of ifs above. */
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

/* Shifts and addition/subtraction in one helper. Shifts on a signed long long
   are implementation-defined in C for negative values, but all targets this
   shell runs on (x86-64, arm64) use arithmetic shifts, which is what bash
   does too -- so we take the same shortcut. */
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

/* Mul/div/mod for the precedence-climbing path, mirroring the old helpers5.c
   logic exactly. Multiplication never overflows in a defined way so we skip
   the check; for div and mod we guard /0 (error unless in a dead branch) and
   the LLONG_MIN / -1 signed-overflow corner case. */
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

/* Dispatch a binary operator to the right helper group. This is the sole
   apply function used by arith_parse_binop; splitting into sub-helpers keeps
   each sub-function within the 42 line-count norm. */
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

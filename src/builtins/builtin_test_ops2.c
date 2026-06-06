/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test_ops2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Integer comparison operators. The names are mnemonic for the POSIX names
   (gt = greater than, ge = ≥, lt = less than, le = ≤) but the operands are
   the already-parsed longs, not strings — parsing is done in test_int_op.
   Returns 2 and prints an error on an unrecognised operator. */
static int	test_int_cmp(const char *op, long a, long b)
{
	if (ft_strcmp(op, "-gt") == 0)
		return (a <= b);
	if (ft_strcmp(op, "-ge") == 0)
		return (a < b);
	if (ft_strcmp(op, "-lt") == 0)
		return (a >= b);
	if (ft_strcmp(op, "-le") == 0)
		return (a > b);
	ft_eprintf("test: %s: unknown operator\n", op);
	return (2);
}

/* Parse both operands as integers, then dispatch to -eq/-ne or test_int_cmp.
   ft_atoi silently returns 0 for non-numeric strings — that is intentional:
   bash does the same so `[ "foo" -eq 0 ]` succeeds rather than erroring. */
int	test_int_op(const char *op, const char *s1, const char *s2)
{
	long	a;
	long	b;

	a = ft_atoi(s1);
	b = ft_atoi(s2);
	if (ft_strcmp(op, "-eq") == 0)
		return (a != b);
	if (ft_strcmp(op, "-ne") == 0)
		return (a == b);
	return (test_int_cmp(op, a, b));
}

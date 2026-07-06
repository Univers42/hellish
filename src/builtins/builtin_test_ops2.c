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

/* Strict integer parse for test operands: optional blanks, optional sign,
   at least one digit, optional trailing blanks, nothing else. bash rejects
   anything looser with "integer expression expected" and exits 2. */
static bool	test_num(const char *s, long *out)
{
	int	i;
	int	start;

	i = 0;
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (s[i] == '+' || s[i] == '-')
		i++;
	start = i;
	while (s[i] >= '0' && s[i] <= '9')
		i++;
	if (i == start)
		return (false);
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (s[i])
		return (false);
	*out = ft_atol(s);
	return (true);
}

/* Parse both operands as integers (erroring like bash when either is not a
   number), then dispatch to -eq/-ne or test_int_cmp. */
int	test_int_op(const char *op, const char *s1, const char *s2)
{
	long	a;
	long	b;

	if (!test_num(s1, &a))
		return (ft_eprintf(
				"test: %s: integer expression expected\n", s1), 2);
	if (!test_num(s2, &b))
		return (ft_eprintf(
				"test: %s: integer expression expected\n", s2), 2);
	if (ft_strcmp(op, "-eq") == 0)
		return (a != b);
	if (ft_strcmp(op, "-ne") == 0)
		return (a == b);
	return (test_int_cmp(op, a, b));
}

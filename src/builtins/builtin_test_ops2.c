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
#include <limits.h>

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

/* Accumulate decimal digits into *n NEGATIVELY (so LONG_MIN itself parses
   without overflowing), refusing any step that would underflow. Advances
   *i past the digits; false = overflow, or no digit at all. */
static bool	test_num_digits(const char *s, int *i, long *n)
{
	if (!ft_isdigit(s[*i]))
		return (false);
	*n = 0;
	while (ft_isdigit(s[*i]))
	{
		if (*n < (LONG_MIN + (s[*i] - '0')) / 10)
			return (false);
		*n = *n * 10 - (s[*i] - '0');
		(*i)++;
	}
	return (true);
}

/* Strict integer parse for test operands: optional blanks, optional sign,
   at least one digit, optional trailing blanks, nothing else — and the
   value must fit in a long. bash rejects anything looser, including
   overflow (strtoimax + ERANGE), with "integer expression expected" and
   exit status 2, so `[ 99999999999999999999 -eq 1 ]` must error out
   instead of silently wrapping. */
static bool	test_num(const char *s, long *out)
{
	int		i;
	int		neg;
	long	n;

	i = 0;
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	neg = (s[i] == '-');
	if (s[i] == '+' || s[i] == '-')
		i++;
	if (!test_num_digits(s, &i, &n))
		return (false);
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (s[i] || (!neg && n == LONG_MIN))
		return (false);
	*out = n;
	if (!neg)
		*out = -n;
	return (true);
}

/* -t: true when the operand is a file descriptor number naming a tty.
   bash errors out like an integer comparison on a non-numeric (or
   overflowing) operand, but quietly returns false when the value simply
   does not fit in an int — `test -t 12345678910` is 1, not 2. */
int	test_tty_op(const char *arg)
{
	long	n;

	if (!test_num(arg, &n))
		return (ft_eprintf(
				"test: %s: integer expression expected\n", arg), 2);
	if (n != (int)n)
		return (1);
	return (isatty((int)n) == 0);
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

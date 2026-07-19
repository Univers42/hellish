/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test_expr.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Recursive-descent evaluator for the full POSIX/XSI test grammar used when
   test gets 4+ arguments (and for 3 arguments that are not `a op b`):
       or   := and { -o and }
       and  := not { -a not }
       not  := '!' not | primary
       prim := '(' or ')' | a binop b | -X a | a
   The 0=true/1=false/2=error convention of the flat evaluator applies; the
   err flag poisons the whole evaluation so `test 1 -eq 1 -o` still exits 2
   like bash instead of short-circuiting the missing operand away. */

bool	tx_is_binop(const char *s)
{
	static const char	*ops[] = {"=", "==", "!=", "<", ">", "-eq", "-ne",
		"-gt", "-ge", "-lt", "-le", "-nt", "-ot", "-ef", NULL};
	size_t				i;

	i = 0;
	while (ops[i])
	{
		if (ft_strcmp(ops[i], s) == 0)
			return (true);
		i++;
	}
	return (false);
}

/* One primary, by lookahead: parenthesised subexpression, `a op b` when a
   binary operator follows, `-X a` for a unary operator with an operand,
   else the single-token non-empty-string test. */
static int	tx_primary(t_tx *t)
{
	char	**av;
	int		r;

	av = t->av;
	if (t->i >= t->ac)
		return (t->err = 1, 2);
	if (ft_strcmp(av[t->i], "(") == 0)
	{
		t->i++;
		r = tx_or(t);
		if (t->i >= t->ac || ft_strcmp(av[t->i], ")") != 0)
			return (t->err = 1, 2);
		return (t->i++, r);
	}
	if (t->i + 3 <= t->ac && tx_is_binop(av[t->i + 1]))
		return (t->i += 3, tx_test_binary(av + t->i - 3));
	if (av[t->i][0] == '-' && av[t->i][1] && !av[t->i][2]
		&& t->i + 1 < t->ac && !tx_is_binop(av[t->i]))
		return (t->i += 2, tx_test_unary(av + t->i - 2));
	r = (ft_strlen(av[t->i]) == 0);
	return (t->i++, r);
}

static int	tx_not(t_tx *t)
{
	int	r;

	if (t->i < t->ac && ft_strcmp(t->av[t->i], "!") == 0)
	{
		t->i++;
		r = tx_not(t);
		if (r == 2 || t->err)
			return (2);
		return (r == 0);
	}
	return (tx_primary(t));
}

static int	tx_and(t_tx *t)
{
	int	r;
	int	r2;

	r = tx_not(t);
	while (!t->err && r != 2 && t->i < t->ac
		&& ft_strcmp(t->av[t->i], "-a") == 0)
	{
		t->i++;
		r2 = tx_not(t);
		if (r2 == 2 || t->err)
			return (2);
		if (r == 0 && r2 != 0)
			r = r2;
	}
	return (r);
}

int	tx_or(t_tx *t)
{
	int	r;
	int	r2;

	r = tx_and(t);
	while (!t->err && r != 2 && t->i < t->ac
		&& ft_strcmp(t->av[t->i], "-o") == 0)
	{
		t->i++;
		r2 = tx_and(t);
		if (r2 == 2 || t->err)
			return (2);
		if (r != 0 && r2 == 0)
			r = 0;
	}
	return (r);
}

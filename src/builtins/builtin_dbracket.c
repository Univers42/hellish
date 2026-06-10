/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_dbracket.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Recursive-descent parser for [[ … ]] compound conditions.
   Grammar (left-recursive eliminated):
     or      ::= and ('||' and)*
     and     ::= not ('&&' not)*
     not     ::= '!' not | primary
     primary ::= '(' or ')' | <flat test tokens>

   The flat-test fallback in db_primary collects everything up to the next
   &&/||/) and hands it to eval_test — so `[[ -f foo && -d bar ]]` works
   without needing to teach the recursive descent about every test operator.

   Return convention (same as POSIX test): 0 = true, 1 = false, 2 = error.
   The forward declarations below satisfy the mutual-recursion requirement. */

static int	db_or(char **av, int ac, int *i);
static int	db_and(char **av, int ac, int *i);
static int	db_not(char **av, int ac, int *i);
static int	db_primary(char **av, int ac, int *i);

/* A single test primary, or a ( ... ) group. A flat primary spans up to the
   next &&/||/). Every path advances *i (group consumes (, flat scan consumes
   >=1 token, or an empty primary returns 2 and the caller's operator
   consumption advances) — so the mutual recursion can never loop. */
static int	db_primary(char **av, int ac, int *i)
{
	int	start;
	int	r;

	if (*i < ac && ft_strcmp(av[*i], "(") == 0)
	{
		(*i)++;
		r = db_or(av, ac, i);
		if (*i < ac && ft_strcmp(av[*i], ")") == 0)
			(*i)++;
		return (r);
	}
	start = *i;
	while (*i < ac && ft_strcmp(av[*i], "&&") != 0
		&& ft_strcmp(av[*i], "||") != 0 && ft_strcmp(av[*i], ")") != 0)
		(*i)++;
	if (*i == start)
		return (2);
	return (db_eval_flat(av + start, *i - start));
}

/* Unary `!` (right-associative), then a primary. 0=true 1=false 2=error. */
static int	db_not(char **av, int ac, int *i)
{
	int	r;

	if (*i < ac && ft_strcmp(av[*i], "!") == 0)
	{
		(*i)++;
		r = db_not(av, ac, i);
		if (r == 2)
			return (2);
		return (r == 0);
	}
	return (db_primary(av, ac, i));
}

/* Logical AND (left-assoc). The `(*i)++` consuming `&&` guarantees progress. */
static int	db_and(char **av, int ac, int *i)
{
	int	left;
	int	right;

	left = db_not(av, ac, i);
	while (*i < ac && ft_strcmp(av[*i], "&&") == 0)
	{
		(*i)++;
		right = db_not(av, ac, i);
		if (left == 2 || right == 2)
			left = 2;
		else if (left == 0 && right == 0)
			left = 0;
		else
			left = 1;
	}
	return (left);
}

/* Logical OR (left-assoc, lowest precedence). */
static int	db_or(char **av, int ac, int *i)
{
	int	left;
	int	right;

	left = db_and(av, ac, i);
	while (*i < ac && ft_strcmp(av[*i], "||") == 0)
	{
		(*i)++;
		right = db_and(av, ac, i);
		if (left == 2 || right == 2)
			left = 2;
		else if (left == 0 || right == 0)
			left = 0;
		else
			left = 1;
	}
	return (left);
}

/* Validate + strip the matching close bracket, then route: `[[` to the logical
   evaluator, `[`/`test` to the flat single-test evaluator. */
int	eval_bracketed(t_shell *st, char **av, int ac, int dbr)
{
	char	*close;
	int		i;
	int		r;

	close = "]";
	if (dbr)
		close = "]]";
	if (ac < 2 || ft_strcmp(av[ac - 1], close) != 0)
		return (ft_eprintf("%s: %s: missing `%s'\n", st->ctx, av[0], close), 2);
	av += 1;
	ac -= 2;
	if (!dbr)
		return (eval_test(av, ac));
	i = 0;
	r = db_or(av, ac, &i);
	if (r != 2 && i != ac)
		return (ft_eprintf("%s: [[: syntax error\n", st->ctx), 2);
	return (r);
}

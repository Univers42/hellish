/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

int		test_file_op(const char *op, const char *path);
int		test_str_op(const char *op, const char *s1, const char *s2);
int		test_int_op(const char *op, const char *s1, const char *s2);

static int	test_unary(char **av, int ac, int *i)
{
	if (*i + 1 >= ac)
		return (1);
	if (ft_strcmp(av[*i], "-z") == 0)
		return (ft_strlen(av[*i + 1]) != 0);
	if (ft_strcmp(av[*i], "-n") == 0)
		return (ft_strlen(av[*i + 1]) == 0);
	if (av[*i][0] == '-' && av[*i][1] && !av[*i][2])
		return (test_file_op(av[*i], av[*i + 1]));
	return (1);
}

static int	test_binary(char **av, int ac, int *i)
{
	if (*i + 2 >= ac)
		return (1);
	if (ft_strcmp(av[*i + 1], "=") == 0
		|| ft_strcmp(av[*i + 1], "==") == 0
		|| ft_strcmp(av[*i + 1], "!=") == 0)
		return (test_str_op(av[*i + 1], av[*i], av[*i + 2]));
	if (ft_strncmp(av[*i + 1], "-", 1) == 0)
		return (test_int_op(av[*i + 1], av[*i], av[*i + 2]));
	return (1);
}

static int	eval_test_result(char **av, int ac, int i)
{
	int	result;

	if (ac == 1)
	{
		if (ft_strlen(av[i]) > 0)
			result = 0;
		else
			result = 1;
	}
	else if (ac == 2)
		result = test_unary(av, i + ac, &i);
	else if (ac == 3)
		result = test_binary(av, i + ac, &i);
	else
		return (ft_eprintf("test: too many arguments\n"), 2);
	return (result);
}

static int	eval_test(char **av, int ac)
{
	int	i;
	int	negate;
	int	result;

	i = 0;
	negate = 0;
	if (ac == 0)
		return (1);
	if (ft_strcmp(av[i], "!") == 0)
	{
		negate = 1;
		i++;
		ac--;
	}
	result = eval_test_result(av, ac, i);
	if (result == 2)
		return (2);
	if (negate)
		return (result == 0);
	return (result);
}

/* `[` and `[[ ]]` are bracketed forms: their last argument must be the matching
   close (`]` or `]]`). `test` has no brackets. Single-test `[[ ]]` reuses the
   same evaluator; internal &&/|| is not handled (the shell splits on those). */
int	builtin_test(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;
	char	*close;

	(void)state;
	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (ac > 0 && (ft_strcmp(av[0], "[") == 0 || ft_strcmp(av[0], "[[") == 0))
	{
		close = "]";
		if (av[0][1])
			close = "]]";
		if (ac < 2 || ft_strcmp(av[ac - 1], close) != 0)
			return (ft_eprintf("%s: %s: missing `%s'\n", state->ctx,
					av[0], close), 2);
		av += 1;
		ac -= 2;
	}
	else
	{
		av += 1;
		ac -= 1;
	}
	return (eval_test(av, ac));
}

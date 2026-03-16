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
		return (ft_strlen(av[*i + 1]) == 0 ? 0 : 1);
	if (ft_strcmp(av[*i], "-n") == 0)
		return (ft_strlen(av[*i + 1]) != 0 ? 0 : 1);
	if (av[*i][0] == '-' && av[*i][1] && !av[*i][2])
		return (test_file_op(av[*i], av[*i + 1]));
	return (1);
}

static int	test_binary(char **av, int ac, int *i)
{
	if (*i + 2 >= ac)
		return (1);
	if (ft_strcmp(av[*i + 1], "=") == 0
		|| ft_strcmp(av[*i + 1], "!=") == 0)
		return (test_str_op(av[*i + 1], av[*i], av[*i + 2]));
	if (ft_strncmp(av[*i + 1], "-", 1) == 0)
		return (test_int_op(av[*i + 1], av[*i], av[*i + 2]));
	return (1);
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
	if (ac == 1)
		result = (ft_strlen(av[i]) > 0) ? 0 : 1;
	else if (ac == 2)
		result = test_unary(av, i + ac, &i);
	else if (ac == 3)
		result = test_binary(av, i + ac, &i);
	else
		return (ft_eprintf("test: too many arguments\n"), 2);
	if (negate)
		return (result == 0);
	return (result);
}

int	builtin_test(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;

	(void)state;
	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (ac > 0 && ft_strcmp(av[0], "[") == 0)
	{
		if (ac < 2 || ft_strcmp(av[ac - 1], "]") != 0)
		{
			ft_eprintf("%s: [: missing `]'\n", state->ctx);
			return (2);
		}
		av++;
		ac -= 2;
	}
	else
	{
		av++;
		ac--;
	}
	return (eval_test(av, ac));
}

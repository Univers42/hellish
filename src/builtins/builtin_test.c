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

/* All test functions follow the POSIX convention: 0 = true, 1 = false,
   2 = error. This is the *inverse* of C's boolean, which trips everyone up
   at least once. Keep it in mind when reading the returns below. */

/* Evaluate a unary test: -z/-n (string length) or any single-letter -X
   flag (file tests). Returns 1 (false) when the operator is unrecognised or
   the operand is missing rather than crashing. */
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

/* Evaluate a binary test: `a op b`. String ops are = == != and integer
   comparison ops are -eq -ne -gt -ge -lt -le. Unrecognised ops return 1. */
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

/* Route to the right evaluator based on token count: 1 = non-empty string,
   2 = unary op + operand, 3 = binary a op b. Anything else is an error (2).
   Note: `i` is the start index and `ac` is the count — so the effective
   range is av[i..i+ac-1]. */
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

/* Entry point for the flat POSIX test evaluator: handles leading '!' and
   delegates to eval_test_result. Used by both the [ ] and [[ ]] evaluators;
   the [[ ]] one (db_or / db_and / …) calls this for each leaf primary it
   finds, after stripping the surrounding brackets. */
int	eval_test(char **av, int ac)
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

/* `[`, `[[`, and `test`. `[[` runs the logical evaluator (`&&`/`||`/`!`/`( )`);
   `[` and `test` run the flat single-test evaluator. eval_bracketed validates
   the matching close bracket (`]`/`]]`) and strips it. */
int	builtin_test(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;

	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (ac > 0 && av[0][0] == '[' && av[0][1] == '[')
		return (eval_bracketed(state, av, ac, 1));
	if (ac > 0 && ft_strcmp(av[0], "[") == 0)
		return (eval_bracketed(state, av, ac, 0));
	return (eval_test(av + 1, ac - 1));
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_dbracket2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+         */
/*   Created: 2026/06/10 06:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/10 06:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

bool	case_match(const char *s, const char *p);

/* Flat [[ ]] primary.  bash semantics: the right side of ==, = and != is
   a PATTERN (the expander escaped its quoted metacharacters, so quoting
   picks literal vs wildcard), matched with the same matcher case/esac
   uses.  Everything else — unary file/string operators, -eq and friends,
   string < > — behaves exactly like test(1), so it is delegated to
   eval_test.  Plain `[`/test NEVER pattern-matches (POSIX `=` is literal
   string equality there); that is why this lives behind the [[ parser
   only.  Returns the test convention: 0 true, 1 false, 2 error. */
/* [[ -v NAME ]]: true when the variable NAME is set. A [[ ]]-only
   operator, so it lives here rather than in the shared test evaluator;
   the environment is reached through the parked state cell. */
static int	db_isset(char **av, int n)
{
	t_shell	*state;

	if (n != 2 || ft_strcmp(av[0], "-v") != 0)
		return (-1);
	state = *db_state_cell();
	if (state && env_get(&state->env, av[1]))
		return (0);
	return (1);
}

int	db_eval_flat(char **av, int n)
{
	int	r;

	if (n == 3 && (ft_strcmp(av[1], "==") == 0
			|| ft_strcmp(av[1], "=") == 0))
		return (case_match(av[0], av[2]) == false);
	if (n == 3 && ft_strcmp(av[1], "!=") == 0)
		return (case_match(av[0], av[2]) == true);
	if (n == 3 && ft_strcmp(av[1], "=~") == 0)
		return (db_regex_match(av[0], av[2]));
	r = db_isset(av, n);
	if (r >= 0)
		return (r);
	return (eval_test(av, n));
}

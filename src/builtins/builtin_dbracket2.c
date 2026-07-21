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

bool	case_match(const char *s, const char *p);

/* Flat [[ ]] primary.  bash semantics: the right side of ==, = and != is
   a PATTERN (the expander escaped its quoted metacharacters, so quoting
   picks literal vs wildcard), matched with the same matcher case/esac
   uses.  Everything else — unary file/string operators, -eq and friends,
   string < > — behaves exactly like test(1), so it is delegated to
   eval_test.  Plain `[`/test NEVER pattern-matches (POSIX `=` is literal
   string equality there); that is why this lives behind the [[ parser
   only.  Returns the test convention: 0 true, 1 false, 2 error. */
int	db_eval_flat(char **av, int n)
{
	if (n == 3 && (ft_strcmp(av[1], "==") == 0
			|| ft_strcmp(av[1], "=") == 0))
		return (case_match(av[0], av[2]) == false);
	if (n == 3 && ft_strcmp(av[1], "!=") == 0)
		return (case_match(av[0], av[2]) == true);
	if (n == 3 && ft_strcmp(av[1], "=~") == 0)
		return (db_regex_match(av[0], av[2]));
	return (eval_test(av, n));
}

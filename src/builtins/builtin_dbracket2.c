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

#include "ft_glob.h"

bool	case_match(const char *s, const char *p);

/* bash enables extglob while the right side of == / = / != inside [[ ]]
   is parsed and matched (since 4.1), `shopt -s extglob` or not --
   bash-completion's _rl_enabled does `[[ $(bind -v) == *+([[:space:]])on* ]]`
   with nothing armed, and it must match (issue #105). Arm the glob cell
   for just this match and put it back; `=~` is regex land and untouched. */
static bool	db_pattern_match(const char *s, const char *p)
{
	int		saved;
	bool	r;

	saved = *glob_extglob_cell();
	*glob_extglob_cell() = 1;
	r = case_match(s, p);
	*glob_extglob_cell() = saved;
	return (r);
}

/* Flat [[ ]] primary.  bash semantics: the right side of ==, = and != is
   a PATTERN (the expander escaped its quoted metacharacters, so quoting
   picks literal vs wildcard), matched with the same matcher case/esac
   uses.  Everything else — unary file/string operators, -eq and friends,
   string < > — behaves exactly like test(1), so it is delegated to
   eval_test.  Plain `[`/test NEVER pattern-matches (POSIX `=` is literal
   string equality there); that is why this lives behind the [[ parser
   only.  Returns the test convention: 0 true, 1 false, 2 error. */
/* [[ -v NAME ]]: true when the variable NAME is set. `test -v` answers the
   same question (test_isset.c) and both route here through the parked state
   cell, so the two spellings cannot drift -- they used to, this one calling
   env_get (which is true for a declared-but-unset name) and the flat
   evaluator stat'ing a file. */
static int	db_isset(char **av, int n)
{
	if (n != 2 || ft_strcmp(av[0], "-v") != 0)
		return (-1);
	return (!test_var_isset(*db_state_cell(), av[1]));
}

int	db_eval_flat(char **av, int n)
{
	int	r;

	if (n == 3 && (ft_strcmp(av[1], "==") == 0
			|| ft_strcmp(av[1], "=") == 0))
		return (db_pattern_match(av[0], av[2]) == false);
	if (n == 3 && ft_strcmp(av[1], "!=") == 0)
		return (db_pattern_match(av[0], av[2]) == true);
	if (n == 3 && ft_strcmp(av[1], "=~") == 0)
		return (db_regex_match(av[0], av[2]));
	r = db_isset(av, n);
	if (r >= 0)
		return (r);
	return (eval_test(av, n));
}

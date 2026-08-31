/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_isset.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

/* `test -v NAME` -- is NAME set?
**
**     v=1; test -v v          bash: rc 0        hellish: rc 1
**
** `-v` fell through to the file-test branch, which stat'd a file literally
** called "v", found none, and answered "false" -- the same answer it gives
** for a genuinely unset variable.  A guard written as `test -v DEBUG ||
** DEBUG=0` therefore ran its fallback every time and never noticed, which
** is the shape that makes this worth fixing rather than documenting.
**
** `[[ -v x ]]` already worked (builtin_dbracket2.c) and parked the t_shell
** in db_state_cell() to reach the environment.  Reusing that cell is what
** lets the flat evaluator answer the same question the same way, rather
** than growing a second notion of "set".
*/

long	arr_sub_index(t_shell *state, const char *sub, long count);

/* The element form: `test -v 'a[1]'` asks whether that ELEMENT is set, not
   whether the array is -- `a=(1 2); test -v 'a[9]'` is false in bash while
   `test -v a` is true.  `sub` points just past the '[' and `slen` stops
   before the ']', so the subscript arrives already stripped. */
static bool	isset_elem(t_shell *st, char *val, char *sub, int slen)
{
	char	*key;
	char	*cur;
	long	idx;

	key = ft_strndup(sub, (size_t)slen);
	if (assoc_is(val))
	{
		cur = assoc_get(val, key, (int)ft_strlen(key));
		return (xfree(key), xfree(cur), cur != NULL);
	}
	idx = arr_sub_index(st, key, arr_count(val));
	xfree(key);
	if (idx < 0)
		return (false);
	cur = arr_get_idx(val, idx);
	return (xfree(cur), cur != NULL);
}

/* Split NAME[sub] and answer for whichever half was asked about. A name
   with no '[' is set when it has a VALUE: `declare -x LATER` names a
   variable without setting one, and bash reports it unset. */
bool	test_var_isset(t_shell *st, char *name)
{
	char	*br;
	char	*val;
	size_t	n;

	if (!st || !name || !*name)
		return (false);
	br = ft_strchr(name, '[');
	n = ft_strlen(name);
	if (!br || name[n - 1] != ']')
		return (env_get(&st->env, name) && env_expand(st, name));
	*br = '\0';
	val = env_expand(st, name);
	*br = '[';
	if (!val)
		return (false);
	return (isset_elem(st, val, br + 1, (int)(name + n - 1 - br - 1)));
}

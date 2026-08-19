/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   unset.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith.h"
#include "env.h"
#include "builtins_private.h"

/* unset arr[i]: evaluate the subscript, rebuild the value without that
   record. The variable itself survives (possibly as an empty array). */
static bool	unset_array_elem(t_shell *state, char *key, char *br)
{
	char	*res;
	char	*nv;
	t_env	*e;
	long	idx;

	*br = '\0';
	e = env_get(&state->env, key);
	res = arith_expand(state, br + 1, (int)ft_strlen(br + 1) - 1);
	*br = '[';
	if (!e || (!arr_is(e->value) && !assoc_is(e->value)))
		return (xfree(res), res != NULL);
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	xfree(res);
	if (assoc_is(e->value))
		nv = assoc_without(e->value, br + 1, (int)ft_strlen(br + 1) - 1);
	else
		nv = arr_without(e->value, idx);
	xfree(e->value);
	e->value = nv;
	return (true);
}

/* Remove the entry `e` from the env table in-place. We find its slot
   via pointer arithmetic (e - arr), free the strings, then compact the array
   by shifting everything after it one position left. This keeps the table a
   flat contiguous array — no holes — at the cost of O(n) per unset. For the
   typical number of variables that is fine and much simpler than a linked
   list. env_index_mark_dirty() invalidates any cached pointer-by-name index
   so the next lookup does a fresh linear scan. */
static void	env_drop_entry(t_shell *state, t_env *e)
{
	t_env	*arr;
	size_t	idx;
	size_t	i;

	arr = (t_env *)state->env.ctx;
	idx = (size_t)(e - arr);
	xfree(arr[idx].key);
	xfree(arr[idx].value);
	i = idx;
	while (i + 1 < state->env.len)
	{
		arr[i] = arr[i + 1];
		i++;
	}
	state->env.len--;
	env_index_mark_dirty();
}

/* Remove one variable.  Returns 1 when the name is read-only: POSIX says
   unset shall not remove such a variable, and bash reports it and returns
   1 (which, unset being a special builtin, then aborts a non-interactive
   shell).  Unsetting a name that simply does not exist is not an error. */
int	try_unset(t_shell *state, char *key)
{
	t_env	*e;
	char	*br;

	if (!state || state->env.ctx == NULL)
		return (0);
	if (is_readonly_var(state, key))
		return (ft_eprintf("%s: unset: %s: cannot unset: readonly "
				"variable\n", state->ctx, key), 1);
	br = ft_strchr(key, '[');
	if (br && key[ft_strlen(key) - 1] == ']'
		&& unset_array_elem(state, key, br))
		return (0);
	e = env_get(&state->env, key);
	if (!e)
		return (0);
	return (env_drop_entry(state, e), 0);
}

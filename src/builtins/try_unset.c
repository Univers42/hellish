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

#include "builtins_private.h"

/* Remove the variable `key` from the env table in-place. We find its slot
   via pointer arithmetic (e - arr), free the strings, then compact the array
   by shifting everything after it one position left. This keeps the table a
   flat contiguous array — no holes — at the cost of O(n) per unset. For the
   typical number of variables that is fine and much simpler than a linked
   list. env_index_mark_dirty() invalidates any cached pointer-by-name index
   so the next lookup does a fresh linear scan. */
void	try_unset(t_shell *state, char *key)
{
	t_env	*e;
	size_t	idx;
	t_env	*arr;
	size_t	i;

	if (!state || state->env.ctx == NULL)
		return ;
	e = env_get(&state->env, key);
	if (!e)
		return ;
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

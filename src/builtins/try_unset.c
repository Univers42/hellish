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
	free(arr[idx].key);
	free(arr[idx].value);
	i = idx;
	while (i + 1 < state->env.len)
	{
		arr[i] = arr[i + 1];
		i++;
	}
	state->env.len--;
}

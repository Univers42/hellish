/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:02:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:02:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

char	*env_expand_n(t_shell *state, char *key, int len)
{
	t_env	*curr;

	if (ft_strncmp(key, "?", len) == 0 && len == 1)
		return (state->last_cmd_status_s);
	else if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
		return (state->pid);
	else if (len == 0)
		return ("");
	curr = env_nget(&state->env, key, len);
	if (curr == 0 || curr->key == 0)
		return (0);
	return (curr->value);
}

char	*env_expand(t_shell *state, char *key)
{
	return (env_expand_n(state, key, ft_strlen(key)));
}

void	env_extend(t_vec_env *dest, t_vec_env *src, bool export)
{
	t_env	curr;

	while (src->len)
	{
		curr = *(t_env *)vec_pop(src);
		if (!curr.key)
			continue ;
		curr.exported = export;
		env_set(dest, curr);
	}
	free(src->ctx);
	vec_init(src);
}

void	env_apply_cmd_assigns(t_shell *state,
			t_executable_cmd *src, bool export)
{
	size_t	i;
	t_env	*el;

	if (!state || !src)
		return ;
	i = 0;
	while (i < src->pre_assigns.len)
	{
		el = &((t_env *)src->pre_assigns.ctx)[i];
		if (!el->key)
		{
			i++;
			continue ;
		}
		el->exported = export;
		env_set(&state->env, *el);
		el->key = NULL;
		el->value = NULL;
		i++;
	}
}

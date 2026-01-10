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

# include "env.h"

char	*env_expand_n(t_state *state, char *key, int len)
{
	t_env	*curr;

	if (ft_strncmp(key, "?", len) == 0 && len == 1)
		return (state->last_cmd_status_s);
	else if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
		return (state->pid);
	else if (len == 0)
		return ("$");
	curr = env_nget(&state->env, key, len);
	if (curr == 0 || curr->key == 0)
		return (0);
	return (curr->value);
}

char	*env_expand(t_state *state, char *key)
{
	return (env_expand_n(state, key, ft_strlen(key)));
}

void	env_extend(t_vec_env *dest, t_vec_env *src, bool export)
{
	t_env	curr;

	while (src->len)
	{
		curr = *(t_env *)vec_pop(src);
		curr.exported = export;
		env_set(dest, curr);
	}
	free(src->ctx);
	vec_init(src);
}


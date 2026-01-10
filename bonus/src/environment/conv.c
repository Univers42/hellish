/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   conv.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:04:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:04:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"

t_env	str_to_env(char *str)
{
	t_env	ret;
	char	*key_pos;

	key_pos = ft_strchr(str, '=') + 1;
	ft_assert(key_pos != 0);
	ret.exported = true;
	ret.key = malloc(key_pos - str);
	ft_strlcpy(ret.key, str, key_pos - str);
	ret.value = ft_strdup(key_pos);
	return (ret);
}

t_vec_env	env_to_vec_env(t_state *state, char **envp)
{
	t_vec_env	ret;

	vec_init(&ret);
	ret.elem_size = sizeof(t_env);
	while (*envp)
	{
		t_env tmp = str_to_env(*envp);
		vec_push(&ret, &tmp);
		envp++;
	}
	if (state->cwd.len)
		env_set(&ret, (t_env){.key = ft_strdup("PWD"),
			.value = ft_strdup((char *)state->cwd.ctx), .exported = true});
	if (state->cwd.len)
		env_set(&ret, (t_env){.key = ft_strdup("IFS"),
			.value = ft_strdup(" \t\n"), .exported = false});
	return (ret);
}


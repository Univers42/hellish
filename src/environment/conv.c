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
	char	*eq;
	size_t	keylen;

	eq = ft_strchr(str, '=');
	ft_assert(eq != 0);
	keylen = (size_t)(eq - str);
	ret.exported = true;
	ret.key = ft_strndup(str, keylen);
	ret.value = ft_strdup(eq + 1);
	return (ret);
}

t_vec_env	env_to_vec_env(t_shell *state, char **envp)
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


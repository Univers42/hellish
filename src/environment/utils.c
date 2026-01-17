/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:02:07 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:02:07 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"

t_env	env_create(char *key, char *value, bool exported)
{
	t_env	e;

	e.key = key;
	e.value = value;
	e.exported = exported;
	return (e);
}

int	env_set(t_vec_env *env, t_env el)
{
	t_env *old;

	ft_assert(el.key != 0);
	old = env_get(env, el.key);
	if (old)
	{
		free(old->value);
		free(el.key);
		old->value = el.value;
		old->exported = el.exported;
	}
	else
	{
		/* push copy of el */
		return (vec_push(env, &el) ? 0 : -1);
	}
	return (0);
}

t_env *env_get(t_vec_env *env, char *key)
{
	t_env *curr;
	size_t i;

	i = 0;
	while (i < env->len)
	{
		curr = &((t_env *)env->ctx)[i];
		if (ft_strcmp(key, curr->key) == 0)
			return (curr);
		i++;
	}
	return (0);
}

// char	**get_envp(t_shell *state, char *exe_path)
// {
// 	char		**ret;
// 	size_t		i;
// 	size_t		j;
// 	t_string	s;

// 	env_set(&state->env, (t_env){.key = ft_strdup("_"),
// 		.value = ft_strdup(exe_path),
// 		.exported = true});
// 	ret = ft_calloc(state->env.len + 1, sizeof(char *));
// 	i = 0;
// 	j = 0;
// 	while (i < state->env.len)
// 	{
// 		t_env *e = &((t_env *)state->env.ctx)[i];
// 		if (e->exported)
// 		{
// 			vec_init(&s);
// 			s.elem_size = 1;
// 			vec_push_str(&s, e->key);
// 			{ char ch = '='; vec_push(&s, &ch); }
// 			vec_push_str(&s, e->value);
// 			ret[j++] = (char *)s.ctx;
// 		}
// 		i++;
// 	}
// 	return (ret);
// }

char **get_envp(t_shell *state, char *exe_path)
{
	(void)exe_path;
	char **ret;
	size_t i;
	size_t j;
	t_string s;

	ret = ft_calloc(state->env.len + 1, sizeof(char *));
	i = 0;
	j = 0;
	while (i < state->env.len)
	{
		t_env *e = &((t_env *)state->env.ctx)[i];
		if (e->exported)
		{
			vec_init(&s);
			s.elem_size = 1;
			vec_push_str(&s, e->key);
			{
				char ch = '=';
				vec_push(&s, &ch);
			}
			vec_push_str(&s, e->value);
			ret[j++] = (char *)s.ctx;
		}
		i++;
	}
	return (ret);
}

t_env *env_nget(t_vec_env *env, char *key, int len)
{
	t_env *curr;
	int i;

	i = env->len - 1;
	while (i >= 0)
	{
		curr = vec_idx(env, i);
		if (ft_strncmp(key, curr->key, len) == 0 && curr->key[len] == 0)
			return (curr);
		i--;
	}
	return (0);
}

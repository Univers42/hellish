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
#include "libft.h"
#include "sys.h"

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
	t_env	*old;

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
		if (vec_push(env, &el))
			return (0);
		else
			return (1);
	}
	return (0);
}

t_env	*env_get(t_vec_env *env, char *key)
{
	t_env	*curr;
	size_t	i;

	i = -1;
	while (++i < env->len)
	{
		curr = &((t_env *)env->ctx)[i];
		if (ft_strcmp(key, curr->key) == 0)
			return (curr);
	}
	return (0);
}

static char	*env_to_str(t_env *e)
{
	t_string	s;
	char		ch;

	vec_init(&s);
	s.elem_size = 1;
	vec_push_str(&s, e->key);
	ch = EQ;
	vec_push(&s, &ch);
	vec_push_str(&s, e->value);
	return ((char *)s.ctx);
}

char	**get_envp(t_shell *state, char *exe_path)
{
	char	**ret;
	size_t	i;
	size_t	j;
	t_env	*e;

	(void)exe_path;
	ret = ft_calloc(state->env.len + 1, sizeof(char *));
	i = -1;
	j = 0;
	while (++i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->exported)
			ret[j++] = env_to_str(e);
	}
	return (ret);
}

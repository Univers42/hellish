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

/* Core env CRUD: create, set (upsert), get, serialise to string, and
   build the envp[] array for execve.  Ownership rule: t_env OWNS its key
   and value strings.  env_set either frees the old value (update path)
   or hands both strings to the vector (insert path). Never double-free. */

#include "env.h"
#include "libft.h"
#include "sys.h"

/* Wrap three fields into a t_env struct.  Caller owns key and value
   (must be heap-allocated, not stack); env_set will eventually free them
   or the caller must free them if env_set is never called. */
t_env	env_create(char *key, char *value, bool exported)
{
	t_env	e;

	e.key = key;
	e.value = value;
	e.exported = exported;
	return (e);
}

/* Upsert: if key already exists, replace value (freeing old) and update
   exported flag; consume el.key (freeing the duplicate).  If new, push
   the whole entry and tell the index about the new slot.  Returns 0 on
   success, 1 if vec_push failed (OOM). */
int	env_set(t_vec_env *env, t_env el)
{
	t_env	*old;

	ft_assert(el.key != 0);
	old = env_get(env, el.key);
	if (old)
	{
		xfree(old->value);
		xfree(el.key);
		old->value = el.value;
		old->exported = el.exported;
	}
	else
	{
		if (!vec_push(env, &el))
			return (1);
		env_index_add(env, (int)env->len - 1);
		return (0);
	}
	return (0);
}

t_env	*env_get(t_vec_env *env, char *key)
{
	int	idx;

	idx = env_index_find(env, key, -1);
	if (idx < 0)
		return (0);
	return (&((t_env *)env->ctx)[idx]);
}

/* Serialise one env entry back to "KEY=VALUE".  Used by get_envp to
   build execve's envp[].  Caller owns the returned string.  One sized
   malloc + two memcpy instead of vector growth: this runs once per
   exported variable for every external command, so the previous
   vec_push_str path paid ~3 reallocs per variable per exec. */
char	*env_to_str(t_env *e)
{
	size_t	klen;
	size_t	vlen;
	char	*s;

	klen = ft_strlen(e->key);
	vlen = 0;
	if (e->value)
		vlen = ft_strlen(e->value);
	s = xmalloc(klen + vlen + 2);
	if (!s)
		return (NULL);
	ft_memcpy(s, e->key, klen);
	s[klen] = EQ;
	if (e->value)
		ft_memcpy(s + klen + 1, e->value, vlen);
	s[klen + 1 + vlen] = '\0';
	return (s);
}

/* Build the NULL-terminated envp[] for execve, including only entries
   where exported==true.  Shell-local variables (exported=false) are
   intentionally hidden from child processes. Caller owns the array and
   each element. */
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

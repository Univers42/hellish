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

/* Conversion between the C-string "KEY=VALUE" format (what execve wants)
   and our internal t_env struct (key+value split, ownership flags).
   Called once at startup to bootstrap the env from the inherited envp[].
   Gotcha: every entry from envp IS exported by definition (that's what it
   means for a variable to be in the environment).  Shell-local variables
   added later carry exported=false. */

#include "env.h"
#include "libft.h"

/* Split "KEY=VALUE" on the '=' sign, duplicate both halves.
   ft_assert blows up loudly if there's no '=' -- malformed envp[] is a
   host bug, not ours, so crashing early beats silently misreading it. */
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

/* Bootstrap a t_vec_env from the host's envp[].  IFS is force-set here
   (not exported) so word-splitting has a sane default even if the parent
   deliberately cleared it.  PWD is overridden with getcwd() so it is
   always accurate -- a parent that chdir'd after setting PWD would give
   us a stale value otherwise. */
t_vec_env	env_to_vec_env(t_shell *state, char **envp)
{
	t_vec_env	ret;
	t_env		tmp;

	vec_init(&ret);
	ret.elem_size = sizeof(t_env);
	env_index_mark_dirty();
	while (*envp)
	{
		tmp = str_to_env(*envp);
		vec_push(&ret, &tmp);
		envp++;
	}
	if (state->cwd.len)
		env_set(&ret, env_create(ft_strdup("PWD"),
				ft_strdup((char *)state->cwd.ctx), true));
	if (state->cwd.len)
		env_set(&ret, env_create(ft_strdup("IFS"),
				ft_strdup(" \t\n"), false));
	return (ret);
}

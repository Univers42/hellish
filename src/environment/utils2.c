/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:29:26 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 15:06:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "../builtins/builtins_private.h"

// this one should be put into env/
void	update_pwd_vars(t_shell *state)
{
	t_env	*pwd;

	pwd = env_get(&state->env, PWD_NAME);
	if (pwd == NULL)
		try_unset(state, OLDPWD_NAME);
	else
		env_set(&state->env, env_create(ft_strdup(OLDPWD_NAME),
				ft_strdup(pwd->value), pwd->exported));
	env_set(&state->env, env_create(ft_strdup(PWD_NAME),
			ft_strdup((char *)state->cwd.ctx), true));
}

/* Like get_envp but also includes non-exported shell variables. Process
   substitutions run as subshells that inherit the parent's shell variables
   (bash semantics), so their child must see them too -- otherwise
   <(echo "$local_var") would expand to nothing. */
char	**get_envp_all(t_shell *state, char *exe_path)
{
	char	**ret;
	size_t	i;
	size_t	j;
	t_env	*e;

	(void)exe_path;
	ret = ft_calloc(state->env.len + 1, sizeof(char *));
	if (!ret)
		return (NULL);
	i = -1;
	j = 0;
	while (++i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->key && e->value)
			ret[j++] = env_to_str(e);
	}
	return (ret);
}

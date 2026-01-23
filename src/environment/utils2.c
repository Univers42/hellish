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

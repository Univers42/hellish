/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_dft_env.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:03:54 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:03:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"
#include "libft.h"
#include "sys.h"

void	set_path(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, PATH);
	if (!e || !e->value || !e->value[0])
	{
		env_set(&state->env, env_create(ft_strdup(PATH),
				ft_strdup(DFT_PATH), true));
	}
}

void	set_underscore(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, ULTIMATE_ARG);
	if (!e || !e->value || !e->value[0])
	{
		if (state->context)
			env_set(&state->env, env_create(ft_strdup(ULTIMATE_ARG),
					ft_strdup(state->context), true));
		env_set(&state->env, env_create(ft_strdup(ULTIMATE_ARG),
				ft_strdup(MINISHELL), true));
	}
}

void	ensure_essential_env_vars(t_shell *state)
{
	char	*cwd;
	t_env	*e;

	cwd = NULL;
	set_home(state);
	set_path(state);
	set_shlvl(state);
	set_underscore(state);
	e = env_get(&state->env, PWD);
	if (!e || !e->value || !e->value[0])
	{
		cwd = getcwd(NULL, 0);
		if (!cwd)
			cwd = ft_strdup(TMP_DIR);
		env_set(&state->env, env_create(ft_strdup(PWD),
				ft_strdup(cwd), true));
		free(cwd);
	}
}

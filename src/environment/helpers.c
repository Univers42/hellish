/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 22:40:02 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 22:40:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"
#include "libft.h"

void	init_cwd(t_shell *state)
{
	char	*cwd;

	vec_init(&state->cwd);
	cwd = getcwd(NULL, 0);
	if (cwd)
		vec_push_str(&state->cwd, cwd);
	else
		ft_eprintf("shell-init: error retrieving current directory:"
			" getcwd: cannot access parent directories:"
			" No such file or directory\nsh: 0: "
			"getcwd() failed: No such file or directory\n");
	free(cwd);
}

void	set_home(t_shell *state)
{
	t_env	*e;
	char	*cwd;

	e = env_get(&state->env, "HOME");
	if (!e || !e->value || !e->value[0])
	{
		cwd = getcwd(NULL, 0);
		if (!cwd)
			cwd = ft_strdup("/tmp");
		env_set(&state->env, env_create(ft_strdup("HOME"),
				ft_strdup(cwd), true));
		free(cwd);
	}
}

void	set_cwd(t_shell *state)
{
	char	*cwd;

	init_cwd(state);
	cwd = getcwd(NULL, 0);
	if (cwd)
		vec_push_str(&state->cwd, cwd);
	else
		ft_eprintf("shell-init: error retrieving current directory:"
			" getcwd: cannot access parent directories:"
			" No such file or directory\nsh: 0: "
			"getcwd() failed: No such file or directory\n");
	free(cwd);
}

void	set_shlvl(t_shell *state)
{
	t_env	*e;
	int		lvl;
	char	buf[16];

	e = env_get(&state->env, "SHLVL");
	if (!e || !e->value || !e->value[0])
	{
		env_set(&state->env, env_create(ft_strdup("SHLVL"),
				ft_strdup("1"), true));
	}
	else
	{
		lvl = 1;
		if (ft_checked_atoi(e->value, &lvl, 42) == 0)
			lvl++;
		else
			lvl = 1;
		snprintf(buf, sizeof(buf), "%d", lvl);
		env_set(&state->env, env_create(ft_strdup("SHLVL"),
				ft_strdup(buf), true));
	}
}

t_env	*env_nget(t_vec_env *env, char *key, int len)
{
	t_env	*curr;
	int		i;

	i = env->len - 1;
	while (i >= 0)
	{
		curr = vec_idx(env, i);
		if (ft_strncmp(key, curr->key, len) == 0
			&& curr->key[len] == 0)
			return (curr);
		i--;
	}
	return (0);
}

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

# include "shell.h"
# include "env.h"
# include "libft.h"

void init_cwd(t_shell *state)
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
		env_set(&state->env, env_create(ft_strdup("HOME"), ft_strdup(cwd), true));
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
	if (!e || !e->value || !e->value[0]) {
		env_set(&state->env, env_create(ft_strdup("SHLVL"), ft_strdup("1"), true));
	} else {
		lvl = 1;
		if (ft_checked_atoi(e->value, &lvl, 42) == 0)
			lvl++;
		else
			lvl = 1;
		snprintf(buf, sizeof(buf), "%d", lvl);
		env_set(&state->env, env_create(ft_strdup("SHLVL"), ft_strdup(buf), true));
	}
}

void	set_path(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "PATH");
	if (!e || !e->value || !e->value[0])
	{
		env_set(&state->env, env_create(ft_strdup("PATH"),
			ft_strdup("/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"), true));
	}
}

void	set_underscore(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "_");
	if (!e || !e->value || !e->value[0])
	{
		if (state->context)
			env_set(&state->env, env_create(ft_strdup("_"), ft_strdup(state->context), true));
		env_set(&state->env, env_create(ft_strdup("_"), ft_strdup("./minishell"), true));
	}
}

void	ensure_essential_env_vars(t_shell *state)
{
	char	*cwd = NULL;
	t_env	*e;

	set_home(state);
	set_path(state);
	set_shlvl(state);
	set_underscore(state);

	e = env_get(&state->env, "PWD");
	if (!e || !e->value || !e->value[0]) {
		cwd = getcwd(NULL, 0);
		if (!cwd)
			cwd = ft_strdup("/tmp");
		env_set(&state->env, env_create(ft_strdup("PWD"), ft_strdup(cwd), true));
		free(cwd);
	}
}

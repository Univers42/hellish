/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd.c                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"
#include <stdlib.h>

static void	update_pwd_vars(t_state *state)
{
	t_env	*pwd;

	pwd = env_get(&state->env, "PWD");
	if (pwd == NULL)
		try_unset(state, "OLDPWD");
	else
	{
		env_set(&state->env, (t_env){.exported = pwd->exported,
			.key = ft_strdup("OLDPWD"), .value = ft_strdup(pwd->value)});
	}
	env_set(&state->env, (t_env){.exported = true, .key = ft_strdup("PWD"),
		.value = ft_strdup((char *)state->cwd.ctx)});
}

static int	cd_check_args(t_state *state, t_vec argv)
{
	if (argv.len >= 3)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			((char **)argv.ctx)[0]);
		return (1);
	}
	return (0);
}

static int	cd_do_chdir(t_state *state, t_vec argv, int *e)
{
	char	*oldpwd;

	oldpwd = env_expand(state, "OLDPWD");
	if (argv.len == 1)
	{
		return (cd_home(e, state));
	}
	if (argv.len >= 2 && !ft_strcmp("-", ((char **)argv.ctx)[1]))
	{
		if (oldpwd == NULL)
		{
			ft_eprintf("%s: cd: OLDPWD not set\n", state->context);
			return (1);
		}
		ft_printf("%s\n", oldpwd);
		*e = chdir(oldpwd);
	}
	else if (argv.len == 2)
	{
		*e = chdir(((char **)argv.ctx)[1]);
	}
	return (0);
}

static void	cd_refresh_cwd(t_state *state, t_vec argv, char *cwd)
{
	if (cwd)
	{
		state->cwd.len = 0;
		vec_push_str(&state->cwd, cwd);
	}
	else
	{
		ft_eprintf("cd: error retrieving current directory: getcwd:"
			" cannot access parent directories:"
			" No such file or directory \n");
		if (!vec_str_ends_with_str(&state->cwd, "/") && state->cwd.ctx)
			vec_push_str(&state->cwd, "/");
		if (argv.len == 2)
			vec_push_str(&state->cwd, ((char **)argv.ctx)[1]);
	}
}

int	builtin_cd(t_state *state, t_vec argv)
{
	char	*cwd;
	int		e;
	char	*arg;

	arg = "";
	e = 0;
	if (cd_check_args(state, argv))
		return (1);
	if (cd_do_chdir(state, argv, &e))
		return (1);
	if (argv.len >= 2)
		arg = ((char **)argv.ctx)[1];
	if (e == -1)
	{
		ft_eprintf("%s: %s: %s: %s\n", state->context, ((char **)argv.ctx)[0], arg,
			strerror(errno));
		return (1);
	}
	cwd = getcwd(NULL, 0);
	cd_refresh_cwd(state, argv, cwd);
	free(cwd);
	update_pwd_vars(state);
	return (0);
}

// norminetear

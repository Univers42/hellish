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

#include "builtins_private.h"

static void	update_pwd_vars(t_shell *state)
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

static bool	is_redir_operator(char *s)
{
	int	i;

	if (!s || !*s)
		return (false);
	if (*s == '<' || *s == '>')
		return (true);
	i = 0;
	while (ft_isdigit((unsigned char)s[i]))
		i++;
	if (i > 0 && (s[i] == '<' || s[i] == '>'))
		return (true);
	return (false);
}

static int	count_real_args(t_vec argv)
{
	size_t	i;
	int		count;
	char	*arg;
	bool	skip_next;

	count = 0;
	skip_next = false;
	i = 1;
	while (i < argv.len)
	{
		arg = ((char **)argv.ctx)[i];
		if (skip_next)
		{
			skip_next = false;
			i++;
			continue ;
		}
		if (is_redir_operator(arg))
		{
			int j = 0;
			while (ft_isdigit((unsigned char)arg[j]))
				j++;
			if (arg[j] == '<' || arg[j] == '>')
			{
				j++;
				if (arg[j] == '>' || arg[j] == '<')
					j++;
			}
			if (arg[j] == '\0')
				skip_next = true;
			i++;
			continue ;
		}
		count++;
		i++;
	}
	return (count);
}

static char	*get_first_real_arg(t_vec argv)
{
	size_t	i;
	char	*arg;
	bool	skip_next;

	skip_next = false;
	i = 1;
	while (i < argv.len)
	{
		arg = ((char **)argv.ctx)[i];
		if (skip_next)
		{
			skip_next = false;
			i++;
			continue ;
		}
		if (is_redir_operator(arg))
		{
			int j = 0;
			while (ft_isdigit((unsigned char)arg[j]))
				j++;
			if (arg[j] == '<' || arg[j] == '>')
			{
				j++;
				if (arg[j] == '>' || arg[j] == '<')
					j++;
			}
			if (arg[j] == '\0')
				skip_next = true;
			i++;
			continue ;
		}
		return (arg);
	}
	return (NULL);
}

static int	cd_check_args(t_vec argv)
{
	int	real_args;

	real_args = count_real_args(argv);
	if (real_args >= 2)
		return (1);
	return (0);
}

static int	cd_do_chdir(t_shell *state, t_vec argv, int *e)
{
	char	*oldpwd;
	char	*path;

	oldpwd = env_expand(state, "OLDPWD");
	path = get_first_real_arg(argv);
	if (!path)
		return (cd_home(e, state));
	if (!ft_strcmp("-", path))
	{
		if (oldpwd == NULL)
		{
			ft_eprintf("%s: cd: OLDPWD not set\n", state->context);
			return (1);
		}
		ft_printf("%s\n", oldpwd);
		*e = chdir(oldpwd);
	}
	else
		*e = chdir(path);
	return (0);
}

static void	cd_refresh_cwd(t_shell *state, t_vec argv, char *cwd)
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

int	builtin_cd(t_shell *state, t_vec argv)
{
	char	*cwd;
	int		e;
	char	*arg;

	e = 0;
	if (cd_check_args(argv))
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			((char **)argv.ctx)[0]);
		return (1);
	}
	if (cd_do_chdir(state, argv, &e))
		return (1);
	arg = get_first_real_arg(argv);
	if (!arg)
		arg = "";
	if (e == -1)
	{
		ft_eprintf("%s: %s: %s: %s\n", state->context,
			((char **)argv.ctx)[0], arg, strerror(errno));
		return (1);
	}
	cwd = getcwd(NULL, 0);
	cd_refresh_cwd(state, argv, cwd);
	free(cwd);
	update_pwd_vars(state);
	return (0);
}

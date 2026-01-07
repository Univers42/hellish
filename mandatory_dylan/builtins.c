/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 23:54:05 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 23:54:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"

int	parse_flags(t_vec_str argv, int *n, int *e)
{
	size_t	i;
	size_t	j;

	i = 1;
	while (i < argv.len && argv.buff[i][0] == '-' && argv.buff[i][1])
	{
		j = 1;
		while (argv.buff[i][j] && ft_strchr("nEe", argv.buff[i][j]))
			j++;
		if (argv.buff[i][j])
			break ;
		j = 1;
		while (argv.buff[i][j])
		{
			if (argv.buff[i][j] == 'n')
				*n = 1;
			if (argv.buff[i][j] == 'e')
				*e = 1;
			if (argv.buff[i][j] == 'E')
				*e = 0;
			j++;
		}
		i++;
	}
	return (i);
}

int	print_args(int e, t_vec_str argv, size_t i)
{
	while (i < argv.len)
	{
		if (e)
		{
			if (e_parser(argv.buff[i]))
				return (1);
		}
		else
			ft_printf("%s", argv.buff[i]);
		if (++i < argv.len)
			ft_printf(" ");
	}
	return (0);
}


void	update_pwd_vars(t_state *state)
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
		.value = ft_strdup(state->cwd.buff)});
}

int	cd_check_args(t_state *state, t_vec_str argv)
{
	if (argv.len >= 3)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			argv.buff[0]);
		return (1);
	}
	return (0);
}

int	cd_do_chdir(t_state *state, t_vec_str argv, int *e)
{
	char	*oldpwd;

	oldpwd = env_expand(state, "OLDPWD");
	if (argv.len == 1)
	{
		return (cd_home(e, state));
	}
	if (argv.len >= 2 && !ft_strcmp("-", argv.buff[1]))
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
		*e = chdir(argv.buff[1]);
	}
	return (0);
}

void	cd_refresh_cwd(t_state *state, t_vec_str argv, char *cwd)
{
	if (cwd)
	{
		state->cwd.len = 0;
		dyn_str_pushstr(&state->cwd, cwd);
	}
	else
	{
		ft_eprintf("cd: error retrieving current directory: getcwd:"
			" cannot access parent directories:"
			" No such file or directory \n");
		if (!dyn_str_ends_with_str(&state->cwd, "/") && state->cwd.buff)
			dyn_str_pushstr(&state->cwd, "/");
		if (argv.len == 2)
			dyn_str_pushstr(&state->cwd, argv.buff[1]);
	}
}

void	parse_export_arg(char *str, char **ident, char **val)
{
	char	*eq;

	eq = ft_strchr(str, '=');
	if (eq)
	{
		*ident = ft_strndup(str, eq - str);
		*val = ft_strdup(eq + 1);
	}
	else
	{
		*ident = ft_strdup(str);
		*val = NULL;
	}
}

static int	process_arg(t_state *st, t_vec_str av, int i)
{
	char	*id;
	t_env	*e;
	char	*val;

	parse_export_arg(av.buff[i], &id, &val);
	if (is_valid_ident(id))
	{
		if (!val)
		{
			e = env_get(&st->env, id);
			if (e)
				e->exported = true;
			free(id);
		}
		else
			env_set(&st->env, (t_env){true, id, val});
	}
	else
	{
		ft_eprintf("%s: %s: `%s' not valid identifier\n", st->context,
			av.buff[0], av.buff[i]);
		free(id);
		return (free(val), 1);
	}
	return (0);
}

int	cd_home(int *e, t_state *state)
{
	char	*home;

	home = env_expand(state, "HOME");
	if (home == NULL)
	{
		ft_eprintf("%s: cd: HOME not set\n", state->context);
		return (1);
	}
	*e = chdir(home);
	return (0);
}

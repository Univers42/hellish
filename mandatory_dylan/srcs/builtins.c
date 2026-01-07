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

/* Forward declarations for helper functions */
static int	parse_flags(t_strings argv, int *n, int *e);
static int	print_args(int e, t_strings argv, size_t i);
static void	update_pwd_vars(t_ctx_builtin *state);
static int	cd_check_args(t_ctx_builtin *state, t_strings argv);
static int	cd_do_chdir(t_ctx_builtin *state, t_strings argv, int *e);
static void	cd_refresh_cwd(t_ctx_builtin *state, t_strings argv, char *cwd);
static int	process_arg(t_ctx_builtin *state, t_strings av, int i);
static int	cd_home(int *e, t_ctx_builtin *state);

static inline int	parse_flags(t_strings argv, int *n, int *e)
{
	size_t	i;
	size_t	j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-' && ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j] && ft_strchr("nEe", ((char **)argv.ctx)[i][j]))
			j++;
		if (((char **)argv.ctx)[i][j])
			break ;
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'n')
				*n = 1;
			if (((char **)argv.ctx)[i][j] == 'e')
				*e = 1;
			if (((char **)argv.ctx)[i][j] == 'E')
				*e = 0;
			j++;
		}
		i++;
	}
	return (i);
}

static inline int	print_args(int e, t_strings argv, size_t i)
{
	while (i < argv.len)
	{
		if (e)
		{
			if (e_parser(((char **)argv.ctx)[i]))
				return (1);
		}
		else
			ft_printf("%s", ((char **)argv.ctx)[i]);
		if (++i < argv.len)
			ft_printf(" ");
	}
	return (0);
}

static inline void	update_pwd_vars(t_ctx_builtin *state)
{
	/* TODO: implement with proper env API */
	(void)state;
}

static inline int	cd_check_args(t_ctx_builtin *state, t_strings argv)
{
	if (argv.len >= 3)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			((char **)argv.ctx)[0]);
		return (1);
	}
	return (0);
}

static inline int	cd_do_chdir(t_ctx_builtin *state, t_strings argv, int *e)
{
	/* TODO: implement with proper env API */
	(void)state;
	(void)argv;
	(void)e;
	return (0);
}

static inline void	cd_refresh_cwd(t_ctx_builtin *state, t_strings argv, char *cwd)
{
	/* TODO: implement */
	(void)state;
	(void)argv;
	(void)cwd;
}

static int	process_arg(t_ctx_builtin *state, t_strings av, int i)
{
	/* TODO: implement */
	(void)state;
	(void)av;
	(void)i;
	return (0);
}

static inline int	cd_home(int *e, t_ctx_builtin *state)
{
	/* TODO: implement */
	(void)e;
	(void)state;
	return (0);
}

/* ============== Builtin implementations ============== */

int	builtin_echo(t_ctx_builtin *state, t_strings argv)
{
	int		n;
	int		e;
	size_t	first_arg_print;

	n = 0;
	e = 0;
	(void)state;
	first_arg_print = parse_flags(argv, &n, &e);
	if (!print_args(e, argv, first_arg_print) && !n)
		ft_printf("\n");
	return (ST_OK);
}

int	builtin_export(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;
	int		status;

	i = 1;
	status = 0;
	if (argv.len == 1)
		return (builtin_env(state, argv));
	while (i < argv.len)
	{
		status = process_arg(state, argv, i) || status;
		i++;
	}
	return (status);
}

int	builtin_cd(t_ctx_builtin *state, t_strings argv)
{
	char	*cwd;
	int		e;
	char	*arg;

	arg = "";
	e = 0;
	if (!cd_check_args(state, argv))
		return (ST_BASE_ERR);
	if (!cd_do_chdir(state, argv, &e))
		return (ST_BASE_ERR);
	if (argv.len >= 2)
		arg = ((char **)argv.ctx)[1];
	if (e == -1)
	{
		ft_eprintf("%s: %s: %s: %s\n", state->context, ((char **)argv.ctx)[0], arg, strerror(errno));
		return (ST_BASE_ERR);
	}
	cwd = getcwd(NULL, 0);
	cd_refresh_cwd(state, argv, cwd);
	free(cwd);
	update_pwd_vars(state);
	return (ST_OK);
}

int	builtin_exit(t_ctx_builtin *state, t_strings argv)
{
	/* TODO: implement properly */
	(void)state;
	(void)argv;
	return (0);
}

int	builtin_pwd(t_ctx_builtin *state, t_strings argv)
{
	(void)state;
	(void)argv;
	return (0);
}

int	builtin_env(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;

	i = 0;
	(void)argv;
	while (i < state->env.len)
	{
		i++;
	}
	return (ST_OK);
}

int	builtin_unset(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;

	(void)state;
	i = 1;
	while (i < argv.len)
	{
		i++;
	}
	return (0);
}

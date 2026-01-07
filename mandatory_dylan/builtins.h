/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:54:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:54:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_H
# define BUILTINS_H

# include "common.h"

typedef struct s_ctx_builtin
{
	t_string	cwd;
	char		*context;
	t_env		env;
	char		*pid;
	int			pid;
}	t_ctx_builtin;

typedef int	(*t_builtin_fn)(t_ctx_builtin *state, t_strings argv);

int	builtin_echo(t_ctx_builtin *state, t_strings argv);
int	builtin_export(t_ctx_builtin *state, t_strings argv);
int	builtin_cd(t_ctx_builtin *state, t_strings argv);
int	builtin_exit(t_ctx_builtin *state, t_strings argv);
int	builtin_pwd(t_ctx_builtin *state, t_strings argv);
int	builtin_env(t_ctx_builtin *state, t_strings argv);
int	builtin_unset(t_ctx_builtin *state, t_strings argv);

/* Initialize builtin function dispatch table. */
static inline t_hash	*builtins_init(void)
{
	t_hash	*dispatch;

	dispatch = malloc(sizeof(t_hash));
	if (dispatch == NULL)
		return (NULL);
	if (!hash_init(dispatch, 16))
	{
		free(dispatch);
		return (NULL);
	}
	hash_set(dispatch, "echo", (void *)builtin_echo);
	hash_set(dispatch, "export", (void *)builtin_export);
	hash_set(dispatch, "cd", (void *)builtin_cd);
	hash_set(dispatch, "exit", (void *)builtin_exit);
	hash_set(dispatch, "pwd", (void *)builtin_pwd);
	hash_set(dispatch, "env", (void *)builtin_env);
	hash_set(dispatch, "unset", (void *)builtin_unset);
	return (dispatch);
}

/* Get builtin function by name. */
static inline t_builtin_fn	builtin_func(t_hash *dispatch, char *name)
{
	if (!name || !dispatch)
		return (NULL);
	return ((t_builtin_fn)hash_get(dispatch, name));
}

static inline int	builtin_echo(t_ctx_builtin *state, t_strings argv)
{
	int		e;
	int		n;
	size_t	first_arg_print;

	n = 0;
	e = 0;
	(void)state;
	first_arg_print = parse_flags(argv, &n, &e);
	if (!print_args(e, argv, first_arg_print) && !n)
		ft_printf("\n");
	return (ST_OK);
}

static inline int	builtin_export(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;
	int		status;

	i = 1;
	status = 0;
	if (av.len == 1)
		return (builtin_env(st, av));
	while (i < av.len)
	{
		status = process_arg(st, av, i) || status;
		i++;
	}
	return (status);
}

static inline int	builtin_cd(t_ctx_builtin *state, t_strings argv)
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
		arg = argv.buff[1];
	if (e == -1)
	{
		ft_eprintf("%s: %s: %s: %s\n", state->context, argv.buff[0], arg, strerror(errno));
		return (ST_BASE_ERR);
	}
	cwd = getcwd(NULL, 0);
	cd_refresh_cwd(state, argv, cwd);
	free(cwd);
	update_pwd_vars(state);
	return (ST_OK);
}

static inline int	builtin_exit(t_ctx_builtin *state, t_strings argv)
{
	int	ret;

	if (state->input_method == INPUT_READLINE)
		ft_eprintf("exit\n");
	if (argv.len == 1)
		exit_clean(state, 0);
	if (ft_checked_atoi(argv.buff[1], &ret, 42))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->context, argv.buff[0], argv.buff[1]);
		exit_clean(state, 2);
	}
	if (argv.len >= 3)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context, argv.buff[0]);
		return (1);
	}
	exit_clean(state, ret);
	return (0);
}

static inline int	builtin_pwd(t_ctx_builtin *state, t_strings argv)
{
	(void)state;
	(void)argv;
	return (0);
}

static inline int	builtin_env(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;

	i = 0;
	(void)argv;
	while (i < state->env.len)
	{
		if (state->env.buff[i].exported)
		{
			ft_printf("%s=%s\n", state->env.buff[i].key, state->env.buff[i].value);
			i++;
		}
	}
	return (ST_OK);
}

static inline int	builtin_unset(t_ctx_builtin *state, t_strings argv)
{
	size_t	i;

	i = 1;
	while (i < argv.len)
	{
		try_unset(state, argv.buff[i]);
		i++;
	}
	return (0);
}

#endif

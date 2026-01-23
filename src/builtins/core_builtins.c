/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core_builtins.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:15:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:36:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* replace builtin_export loop with calls to helpers */
int	builtin_export(t_shell *st, t_vec av)
{
	size_t	i;
	int		status;
	int		idx;

	i = 1;
	status = 0;
	if (av.len == 1)
		return (collect_and_print_exported(st), 0);
	while (i < av.len)
	{
		idx = (int)i;
		status = process_arg(st, av, &idx) || status;
		i = (size_t)idx + 1;
	}
	if (status)
		return (1);
	return (0);
}

int	builtin_exit(t_shell *state, t_vec argv)
{
	int		ret;
	size_t	i;

	print_exit_if_readline(state);
	if (handle_no_args(state, argv))
		return (0);
	i = 1;
	i = handle_double_dash(state, argv, i);
	if (handle_non_numeric(state, argv, i, &ret))
		return (0);
	if (handle_too_many_args(state, argv, i))
		return (0);
	return (exit_clean(state, ret), 0);
}

int	builtin_echo(t_shell *state, t_vec argv)
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
	return (0);
}

int	builtin_env(t_shell *state, t_vec argv)
{
	size_t	i;
	t_env	*e;

	i = -1;
	(void)argv;
	while (++i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->exported)
			ft_printf("%s=%s\n", e->key, e->value);
	}
	return (0);
}

int	builtin_unset(t_shell *state, t_vec argv)
{
	size_t	i;

	i = 1;
	while (i < argv.len)
	{
		try_unset(state, ((char **)argv.ctx)[i]);
		i++;
	}
	return (0);
}

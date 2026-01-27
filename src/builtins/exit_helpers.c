/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit_helpers.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:59:39 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:17:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

// Print exit if using readline
void	print_exit_if_readline(t_shell *state)
{
	if (state->metinp == INP_RL)
		ft_eprintf("exit\n");
}

// No arguments: use last command status
int	handle_no_args(t_shell *state, t_vec argv)
{
	if (argv.len == 1)
	{
		exit_clean(state, state->last_cmd_st_exe.status);
		return (1);
	}
	return (0);
}

// Handle optional "--"
size_t	handle_double_dash(t_shell *state, t_vec argv, size_t i)
{
	if (i < argv.len && ft_strcmp(((char **)argv.ctx)[i], "--") == 0)
	{
		if (i + 1 == argv.len)
			exit_clean(state, state->last_cmd_st_exe.status);
		return (i + 1);
	}
	return (i);
}

// First argument must be numeric
int	handle_non_numeric(t_shell *state, t_vec argv, size_t i, int *ret)
{
	if (ft_checked_atoi(((char **)argv.ctx)[i], ret, 42))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->ctx,
			((char **)argv.ctx)[0], ((char **)argv.ctx)[i]);
		return (exit_clean(state, 2), 1);
	}
	return (0);
}

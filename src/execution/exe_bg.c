/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_bg.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:11:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 19:03:01 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

t_exe_res	execute_cmd_bg(t_shell *state,
						t_executable_node *exe, t_executable_cmd *cmd)
{
	int		pid;
	char	*last;

	if (cmd->argv.len > 0)
	{
		last = ((char **)cmd->argv.ctx)[cmd->argv.len - 1];
		if (last)
			env_set(&state->env,
				env_create(ft_strdup(ULTIMATE_ARG), ft_strdup(last), true));
	}
	pid = fork();
	if (pid == 0)
		(default_signal_handlers(), set_up_redirection(state, exe),
			env_extend(&state->env, &cmd->pre_assigns, true),
			exit(actually_run(state, &cmd->argv)));
	(procsub_close_fds_parent(state), free_executable_cmd(*cmd));
	return (free_executable_node(exe), res_pid(pid));
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_builtin.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:11:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:38:09 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

t_execution_state	execute_builtin_cmd_fg(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	int	stdin_bak;
	int	stdout_bak;
	int	status;

	stdin_bak = dup(0);
	stdout_bak = dup(1);
	set_up_redirection(state, exe);
	update_underscore_var(state, cmd);
	status = builtin_func(((char **)(cmd->argv.ctx))[0])(state, cmd->argv);
	dup2(stdin_bak, 0);
	dup2(stdout_bak, 1);
	close(stdin_bak);
	close(stdout_bak);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(status));
}

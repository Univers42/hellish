/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_bg.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:11:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:05:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* Fork and run an external command.  Pre-assignment NAME=val words go
   permanently into the child's environment via env_extend (unlike in
   builtins they do not need to be rolled back -- the child's env dies with
   it).  We set default signal handlers so job-control SIG_IGN settings
   from the parent are cleared before execve.  The parent closes any
   process-substitution fds that were opened for this pipeline stage,
   frees cmd and exe, then returns a res_pid to be waited on by
   pipeline_status or exe_res_set_status. */
t_execution_state	execute_cmd_bg(t_shell *state,
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
	{
		default_signal_handlers();
		set_up_redirection(state, exe);
		env_extend(&state->env, &cmd->pre_assigns, true);
		exit(actually_run(state, &cmd->argv));
	}
	procsub_close_fds_parent(state);
	free_executable_cmd(state, *cmd);
	return (free_executable_node(exe), res_pid(pid));
}

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
/* Do the child's work WITHOUT forking. Only ever reached when this process is
   already a disposable $( ) body running its one and only command (see
   cs_single_cmd), so there is nothing left to return to: we are the child.
   Saves the second clone -- `$(/bin/true)` costs one, like bash and dash.
   Never returns: actually_run execve's, or exits with its failure status. */
static void	run_cmd_in_place(t_shell *state,
					t_executable_node *exe, t_executable_cmd *cmd)
{
	default_signal_handlers();
	set_up_redirection(state, exe);
	env_extend(&state->env, &cmd->pre_assigns, true);
	exit(actually_run(state, &cmd->argv));
}

/* Fork argv as an EXTERNAL program, wait for it, hand back its status.
   `command NAME args` is the caller: it has to reach the external NAME even
   when a shell function shadows it, so it cannot go through actually_run
   (whose function lookup would run the very function `command` exists to
   step around).  Redirections are already in force on this process, so the
   child inherits them across the fork and needs no set_up_redirection. */
int	run_external_sync(t_shell *state, t_vec *args)
{
	t_execution_state	res;
	int					pid;

	pid = fork();
	if (pid < 0)
		return (ft_eprintf("%s: fork: %s\n", state->ctx,
				strerror(errno)), 1);
	if (pid == 0)
	{
		default_signal_handlers();
		_exit(exec_external_argv(state, args));
	}
	res = res_pid(pid);
	exe_res_set_status(state, &res);
	return (res.status);
}

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
	if (state->cmdsub_in_place)
		run_cmd_in_place(state, exe, cmd);
	pid = fork();
	if (pid == 0)
	{
		default_signal_handlers();
		set_up_redirection(state, exe);
		env_extend(&state->env, &cmd->pre_assigns, true);
		_exit(actually_run(state, &cmd->argv));
	}
	procsub_close_fds_parent(state);
	free_executable_cmd(state, *cmd);
	return (free_executable_node(exe), res_pid(pid));
}

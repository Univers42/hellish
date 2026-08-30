/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   res_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* Build an execution result that already carries a concrete exit status.
   pid=-1 signals "no background child to wait for". */
t_execution_state	res_status(int status)
{
	return ((t_execution_state){.status = status, .pid = -1});
}

/* Build an execution result that points at a running child.  status=-1
   signals "not yet known -- must waitpid before reading".  Callers call
   exe_res_set_status to block until the child exits and fill in status. */
t_execution_state	res_pid(int pid)
{
	return ((t_execution_state){.status = -1, .pid = pid});
}

/* Block until the child recorded in res->pid exits and decode its wait
   status into res->status.  POSIX signal-killed encoding: 128 + signum.
   We restart on EINTR (the inner while(1)/waitpid loop) so a SIGCHLD
   from an unrelated background job does not cause us to lose track of
   this specific child.  Ctrl-C (SIGINT from the terminal) is noted in
   ctrl_c so the parent can print a newline or exit a script.  Waits go
   through pal_waitpid: the win32 shim resolves the pid to a HANDLE via
   st->pal_procs, which is why the shell handle is threaded in here.

   A job killed BY A SIGNAL never ran its own cleanup, so a program that
   turned echo off to read a password (chsh, ssh, sudo) leaves the terminal
   that way and the shell is the only thing left that can undo it (#85).
   Only on a signal: restoring after every command would reverse an `stty
   -echo` the user typed on purpose. Interactive only -- a script has no
   terminal state of its own to reclaim. */
void	exe_res_set_status(t_shell *st, t_execution_state *res)
{
	int	status;

	if (res->status != -1)
		return ;
	ft_assert(res->pid != -1);
	while (1)
		if (pal_waitpid(st, res->pid, &status, WUNTRACED) != -1)
			break ;
	if (WIFSTOPPED(status))
		return (fg_job_stopped(st, res, status));
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
		res->ctrl_c = true;
	if (WIFSIGNALED(status) && st->metinp == INP_RL)
		tty_reclaim_after_signal();
	res->status = WEXITSTATUS(status)
		+ WIFSIGNALED(status) * 128 + WIFSIGNALED(status) * WTERMSIG(status);
}

/* `set -o pipefail`: wait every child (reaping them) and return the rightmost
   non-zero exit, or 0 if all succeeded. */
static t_execution_state	pipefail_scan(t_shell *st, t_vec_exe_res *results)
{
	t_execution_state	*r;
	t_execution_state	res;
	size_t				i;

	res = res_status(0);
	i = 0;
	while (i < results->len)
	{
		r = (t_execution_state *)vec_idx(results, i);
		if (r->pid != -1)
			exe_res_set_status(st, r);
		if (r->status != 0)
			res = *r;
		i++;
	}
	return (res);
}

/* Exit status of a pipeline: pipefail (above), else the last command.
   Every stage is waited here (bash waits all pipeline members before
   moving on) and the statuses are recorded into PIPESTATUS. */
t_execution_state	pipeline_status(t_shell *state, t_vec_exe_res *results)
{
	t_execution_state	*r;
	size_t				i;

	i = 0;
	while (i < results->len)
	{
		r = (t_execution_state *)vec_idx(results, i);
		if (r->pid != -1)
			exe_res_set_status(state, r);
		i++;
	}
	set_pipestatus(state, results);
	if (state->opt_pipefail)
		return (pipefail_scan(state, results));
	return (*(t_execution_state *)vec_idx(results, results->len - 1));
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_fg.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>

static int	fg_wait_result(t_shell *state, t_job *job, int status)
{
	if (WIFSTOPPED(status))
	{
		job->status = JOB_STOPPED;
		job_print(job, state->job_table.current,
			state->job_table.previous, false);
		return (148);
	}
	job->status = JOB_DONE;
	job_remove(&state->job_table, job->id);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (128 + WTERMSIG(status));
}

int	builtin_fg(t_shell *state, t_vec argv)
{
	t_job	*job;
	char	*spec;
	int		status;

	spec = NULL;
	if (argv.len > 1)
		spec = ((char **)argv.ctx)[1];
	job = job_by_spec(&state->job_table, spec);
	if (!job)
	{
		if (spec)
			ft_eprintf("%s: fg: %s: no such job\n", state->ctx, spec);
		else
			ft_eprintf("%s: fg: current: no such job\n", state->ctx);
		return (1);
	}
	ft_printf("%s\n", job->cmd);
	job->bg = false;
	job->status = JOB_RUNNING;
	kill(-job->pgid, SIGCONT);
	tcsetpgrp(STDIN_FILENO, job->pgid);
	waitpid(-job->pgid, &status, WUNTRACED);
	tcsetpgrp(STDIN_FILENO, getpgrp());
	return (fg_wait_result(state, job, status));
}

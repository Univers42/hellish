/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_bg.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <signal.h>

/* Validate that a job exists and is currently stopped before resuming it in
   the background. A job already running in the background cannot be bg'd
   again — report an error rather than sending a redundant SIGCONT. */
static int	bg_check_job(t_shell *state, t_job *job, char *spec)
{
	if (!job)
	{
		if (spec)
			ft_eprintf("%s: bg: %s: no such job\n", state->ctx, spec);
		else
			ft_eprintf("%s: bg: current: no such job\n", state->ctx);
		return (1);
	}
	if (job->status != JOB_STOPPED)
	{
		ft_eprintf("%s: bg: job %d already in background\n",
			state->ctx, job->id);
		return (1);
	}
	return (0);
}

/* bg [%job]: resume a stopped job in the background with SIGCONT. The `[n]+`
   vs `[n]-` prefix mimics bash: `+` for the current job, `-` for the
   previous. We do NOT take the terminal away (no tcsetpgrp) since the job
   runs in the background — it will get SIGTTOU if it tries to write to the
   terminal while we own it. */
int	builtin_bg(t_shell *state, t_vec argv)
{
	t_job	*job;
	char	*spec;

	spec = NULL;
	if (argv.len > 1)
		spec = ((char **)argv.ctx)[1];
	job = job_by_spec(&state->job_table, spec);
	if (bg_check_job(state, job, spec))
		return (1);
	job->bg = true;
	job->status = JOB_RUNNING;
	if (job->id == state->job_table.current)
		ft_printf("[%d]+ %s &\n", job->id, job->cmd);
	else
		ft_printf("[%d]- %s &\n", job->id, job->cmd);
	kill(-job->pgid, SIGCONT);
	return (0);
}

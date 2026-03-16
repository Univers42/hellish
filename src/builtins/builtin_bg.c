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

int	builtin_bg(t_shell *state, t_vec argv)
{
	t_job	*job;
	char	*spec;

	spec = NULL;
	if (argv.len > 1)
		spec = ((char **)argv.ctx)[1];
	job = job_by_spec(&state->job_table, spec);
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
	job->bg = true;
	job->status = JOB_RUNNING;
	ft_printf("[%d]%c %s &\n", job->id,
		(job->id == state->job_table.current) ? '+' : '-', job->cmd);
	kill(-job->pgid, SIGCONT);
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Job display and notification helpers.  job_print is the single place
   that formats a job line (used by both `jobs` and the async Done
   notification).  job_notify is called at each prompt: it polls, prints
   any finished jobs, then removes them.  The notified flag prevents
   printing the same "Done" line twice if the prompt is re-drawn quickly. */

#include "job_control.h"
#include "shell.h"
#include "libft.h"
#include <stdio.h>

/* Translate a t_job_status enum to the human string that `jobs` shows. */
static const char	*job_status_str(t_job_status s)
{
	if (s == JOB_RUNNING)
		return ("Running");
	if (s == JOB_STOPPED)
		return ("Stopped");
	if (s == JOB_DONE)
		return ("Done");
	if (s == JOB_KILLED)
		return ("Killed");
	return ("Unknown");
}

/* Print one job in `jobs` format.  current/prev are the job IDs (not
   slot indices) that get '+'/'-' markers.  show_pid adds the pgid column,
   which `jobs -l` requests but the async notification doesn't need. */
void	job_print(t_job *job, int current, int prev, bool show_pid)
{
	char	mark;

	mark = ' ';
	if (job->id == current)
		mark = '+';
	else if (job->id == prev)
		mark = '-';
	if (show_pid)
		ft_printf("[%d]%c  %d %-24s%s\n",
			job->id, mark, job->pgid, job_status_str(job->status), job->cmd);
	else
		ft_printf("[%d]%c  %-24s%s\n",
			job->id, mark, job_status_str(job->status), job->cmd);
}

/* Called before each prompt.  Polls all running jobs, then prints and
   removes any that finished.  The notified flag is set to true just
   before removal so the loop can't double-print if job_remove shifts
   the count. */
void	job_notify(t_shell *state)
{
	t_job_table	*jt;
	int			i;

	jt = &state->job_table;
	job_update_status(jt);
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].status == JOB_DONE
			&& !jt->jobs[i].notified)
		{
			job_print(&jt->jobs[i], jt->current, jt->previous, false);
			jt->jobs[i].notified = true;
			job_remove(jt, jt->jobs[i].id);
		}
		i++;
	}
}

/* Promote `id` to current job, demoting the old current to previous.
   Called when `fg` or `bg` brings a specific job to the foreground so
   the '+'/'-' markers in `jobs` output stay correct. */
void	job_set_current(t_job_table *jt, int id)
{
	jt->previous = jt->current;
	jt->current = id;
}

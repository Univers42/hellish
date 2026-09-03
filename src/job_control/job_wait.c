/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_wait.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "job_control.h"

/* What a bare `wait` reports, and what it leaves for `jobs`.
**
** bash retires a job once its status has been REPORTED, and a bare `wait`
** reports only the jobs it reaped itself. A job that had already died --
** reaped by the SIGCHLD path between two commands, never printed -- is
** left in the table, and the next `jobs` says "Terminated" or "Done" for
** it. hellish's `wait` retired every finished job instead, so
**
**     sleep 0.3 & kill %1; wait; jobs
**
** printed the Terminated line on a slow runner (job dead before `wait`
** ran: bash keeps it) and nothing on a fast one (dead during `wait`:
** bash drops it), and hellish printed nothing either way. The arm64 CI
** rung sat on the slow side of that race every other push.
*/

/* The job `pid` leads has just been reaped by `wait` itself: that counts
   as reported. */
void	job_mark_reported(t_job_table *jt, pid_t pid)
{
	t_job	*job;

	job = job_find_pgid(jt, pid);
	if (job)
		job->notified = true;
}

/* Every job that was already dead when a non-interactive `wait` began
   counts as reported -- EXCEPT the one `$!` names. bash's
   mark_dead_jobs_as_notified skips last_asynchronous_pid in a script, so
   after `a & b & wait`, `jobs` still prints b's Done line and nothing
   about a; and a job that is not `$!` gives its number back at once. */
void	job_mark_reported_except(t_job_table *jt, pid_t keep)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && job_finished(&jt->jobs[i])
			&& jt->jobs[i].pgid != keep)
			jt->jobs[i].notified = true;
		i++;
	}
}

/* Retire the finished jobs that have been reported -- by this `wait`, or
   by an earlier `jobs` -- and nothing else. */
void	job_purge_reported(t_job_table *jt)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && job_finished(&jt->jobs[i])
			&& jt->jobs[i].notified)
			job_remove(jt, jt->jobs[i].id);
		i++;
	}
}

/* The lowest-numbered job still running or stopped -- the one a bare
   `wait` is actively waiting on, in bash's table order -- or NULL. */
t_job	*job_first_unfinished(t_job_table *jt)
{
	int		id;
	t_job	*job;

	id = job_next_after(jt, 0);
	while (id)
	{
		job = job_find_id(jt, id);
		if (job && !job_finished(job))
			return (job);
		id = job_next_after(jt, id);
	}
	return (NULL);
}

/* bash's reset_current candidate: the highest-numbered stopped job, else
   the highest-numbered running one, else -1. A finished job is never
   elected -- which is why a lone Done entry prints with no `+` after a
   `wait` retired its neighbour. */
int	job_live_id(t_job_table *jt, int not_this)
{
	int				i;
	int				best;
	t_job_status	want;

	want = JOB_STOPPED;
	while (1)
	{
		best = -1;
		i = 0;
		while (i < JOB_MAX)
		{
			if (jt->jobs[i].pgid && jt->jobs[i].id != not_this
				&& jt->jobs[i].id > best && jt->jobs[i].status == want)
				best = jt->jobs[i].id;
			i++;
		}
		if (best >= 0 || want == JOB_RUNNING)
			return (best);
		want = JOB_RUNNING;
	}
}

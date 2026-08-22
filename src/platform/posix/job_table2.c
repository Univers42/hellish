/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_table2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Non-blocking status poll (WNOHANG) and table teardown.
   job_update_status is called at the top of each prompt cycle (in
   job_notify) to catch background jobs that finished while we were
   reading the previous command.  Signal-killed exit codes follow the
   128+signal convention used by bash. */

#include "job_control.h"
#include "shell.h"
#include "libft.h"
#include <stdlib.h>
#include <sys/wait.h>

/* File one reaped child.  A TERMINATED child goes through bg_done_record so
   the status lands in the bg_done ring as well as in the job table; a
   stopped or continued one only updates the table, because neither is
   something `wait` can collect.

   The ring is the durable half.  This poll used to record into the job
   table ALONE, and a table entry does not survive: any `wait` on an
   unrelated pid ends with job_purge_done, which retires every finished job
   -- including one whose status nobody has collected yet.  A pid reaped
   here and purged there is then unknown to both lookups in
   reaped_job_status, and `wait` on it answers 127 instead of its real exit
   code.  builtin_jobs already calls bg_done_record for exactly this
   reason; the poll simply had not been taught the same rule.

   NOTE: tests/hard/12_job_control.sh fails intermittently (~1-2%, load
   dependent) with `wait $p3` reporting 127, which is the shape this gap
   would produce.  That flake predates this change and has NOT been proven
   to be this gap -- it did not reproduce in 80 runs with this fix reverted.
   Treat the durability rule as correct on its own terms, not as a closed
   diagnosis of that flake. */
static void	job_reap_record(t_shell *st, t_job *job, int status)
{
	if (WIFSTOPPED(status) || WIFCONTINUED(status))
		return (job_record_exit(job, status));
	bg_done_record(st, job->pgid, status);
}

/* Poll every job that has not finished with
   waitpid(WNOHANG|WUNTRACED|WCONTINUED).  We poll by process GROUP (-pgid)
   so that every process in the pipeline is reaped, not just the leader.
   Called before printing the prompt so the user sees "Done" at the next
   prompt rather than one cycle late.

   STOPPED jobs are polled too, and that is not cosmetic: this used to test
   `status == JOB_RUNNING`, so once a job stopped the shell stopped asking
   about it forever.  A ^Z'd job that was then killed stayed listed as
   "Stopped" for the rest of the session, and `wait` on it never returned.
   WCONTINUED is the other half of the same gap -- without it a job resumed
   by `kill -CONT %1` behind the shell's back still read as stopped. */
void	job_update_status(t_shell *st)
{
	t_job_table	*jt;
	int			i;
	pid_t		pid;
	int			status;

	jt = &st->job_table;
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && !job_finished(&jt->jobs[i]))
		{
			pid = waitpid(-jt->jobs[i].pgid, &status,
					WNOHANG | WUNTRACED | WCONTINUED);
			if (pid > 0)
				job_reap_record(st, &jt->jobs[i], status);
		}
		i++;
	}
}

/* Shell exit cleanup: free every cmd string still alive and zero the
   table.  Calling this avoids false ASan leaks on clean exit. */
void	job_table_free(t_job_table *jt)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid)
			xfree(jt->jobs[i].cmd);
		i++;
	}
	ft_memset(jt, 0, sizeof(t_job_table));
}

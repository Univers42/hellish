/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_table.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Job table CRUD.  A job is a pipeline (or single command) started in
   the background.  Each slot is identified by a monotonically-increasing
   job ID (not recycled) and the process group ID (pgid) of the pipeline.
   pgid==0 means the slot is free.  current/previous track the '+'/'-'
   markers shown by `jobs`.  JOB_MAX is 256 -- more than enough for any
   sane interactive session. */

#include "job_control.h"
#include "libft.h"
#include <stdlib.h>
#include <sys/wait.h>

/* Zero the whole table and set sentinel values: IDs start at 1,
   current/previous at -1 (meaning "no current job yet"). */
void	job_table_init(t_job_table *jt)
{
	ft_memset(jt, 0, sizeof(t_job_table));
	jt->next_id = 1;
	jt->current = -1;
	jt->previous = -1;
}

/* Find a free slot (pgid==0), fill it, update current/previous, and
   increment the running count.  Returns NULL if the table is full
   (JOB_MAX reached -- effectively impossible in practice). */
t_job	*job_add(t_job_table *jt, pid_t pgid, const char *cmd, bool bg)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid == 0)
		{
			jt->jobs[i].id = jt->next_id++;
			jt->jobs[i].pgid = pgid;
			jt->jobs[i].status = JOB_RUNNING;
			jt->jobs[i].cmd = ft_strdup(cmd);
			jt->jobs[i].notified = false;
			jt->jobs[i].bg = bg;
			jt->jobs[i].exit_code = 0;
			jt->count++;
			jt->previous = jt->current;
			jt->current = jt->jobs[i].id;
			return (&jt->jobs[i]);
		}
		i++;
	}
	return (NULL);
}

/* Look up a job by its shell-assigned ID (the [N] shown by `jobs`).
   Skips free slots (pgid==0). */
t_job	*job_find_id(t_job_table *jt, int id)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].id == id)
			return (&jt->jobs[i]);
		i++;
	}
	return (NULL);
}

/* Look up a job by process group ID.  Used in SIGCHLD handlers where
   only the pgid is known from waitpid(). */
t_job	*job_find_pgid(t_job_table *jt, pid_t pgid)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid == pgid)
			return (&jt->jobs[i]);
		i++;
	}
	return (NULL);
}

/* Free the cmd string and zero the slot so it's available for the next
   job.  Does NOT update current/previous -- callers do that when needed
   (e.g. after job_notify prints the "Done" line). */
void	job_remove(t_job_table *jt, int id)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].id == id && jt->jobs[i].pgid)
		{
			xfree(jt->jobs[i].cmd);
			ft_memset(&jt->jobs[i], 0, sizeof(t_job));
			jt->count--;
			return ;
		}
		i++;
	}
}

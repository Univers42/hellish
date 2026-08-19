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
#include "libft.h"
#include <stdlib.h>
#include <sys/wait.h>

/* Poll every running job with waitpid(WNOHANG|WUNTRACED).  We poll by
   process GROUP (-pgid) so that every process in the pipeline is reaped,
   not just the leader.  Called before printing the prompt so the user
   sees "Done" at the next prompt rather than one cycle late. */
void	job_update_status(t_job_table *jt)
{
	int		i;
	pid_t	pid;
	int		status;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].status == JOB_RUNNING)
		{
			pid = waitpid(-jt->jobs[i].pgid, &status, WNOHANG | WUNTRACED);
			if (pid > 0)
				job_record_exit(&jt->jobs[i], status);
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

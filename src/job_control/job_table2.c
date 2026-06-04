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

#include "job_control.h"
#include "libft.h"
#include <stdlib.h>
#include <sys/wait.h>

static void	job_update_entry(t_job *job, int status)
{
	if (WIFSTOPPED(status))
		job->status = JOB_STOPPED;
	else if (WIFEXITED(status) || WIFSIGNALED(status))
	{
		job->status = JOB_DONE;
		if (WIFEXITED(status))
			job->exit_code = WEXITSTATUS(status);
		else
			job->exit_code = 128 + WTERMSIG(status);
	}
}

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
				job_update_entry(&jt->jobs[i], status);
		}
		i++;
	}
}

void	job_table_free(t_job_table *jt)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid)
			free(jt->jobs[i].cmd);
		i++;
	}
	ft_memset(jt, 0, sizeof(t_job_table));
}

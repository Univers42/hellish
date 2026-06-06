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

#include "job_control.h"
#include "libft.h"
#include <stdlib.h>
#include <sys/wait.h>

void	job_table_init(t_job_table *jt)
{
	ft_memset(jt, 0, sizeof(t_job_table));
	jt->next_id = 1;
	jt->current = -1;
	jt->previous = -1;
}

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

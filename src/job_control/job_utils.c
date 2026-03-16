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

#include "job_control.h"
#include "shell.h"
#include "libft.h"
#include <stdio.h>

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

void	job_set_current(t_job_table *jt, int id)
{
	jt->previous = jt->current;
	jt->current = id;
}

static t_job	*job_by_str(t_job_table *jt, const char *str)
{
	int	i;

	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].cmd
			&& ft_strncmp(jt->jobs[i].cmd, str, ft_strlen(str)) == 0)
			return (&jt->jobs[i]);
		i++;
	}
	return (NULL);
}

t_job	*job_by_spec(t_job_table *jt, const char *spec)
{
	int	id;

	if (!spec || !spec[0])
		return (job_find_id(jt, jt->current));
	if (spec[0] == '%')
	{
		if (spec[1] == '%' || spec[1] == '+' || spec[1] == '\0')
			return (job_find_id(jt, jt->current));
		if (spec[1] == '-')
			return (job_find_id(jt, jt->previous));
		if (spec[1] >= '0' && spec[1] <= '9')
		{
			id = ft_atoi(spec + 1);
			return (job_find_id(jt, id));
		}
		return (job_by_str(jt, spec + 1));
	}
	return (NULL);
}

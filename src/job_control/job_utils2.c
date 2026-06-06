/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_utils2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* job_by_spec: resolve a jobspec (the argument to fg/bg/wait) to a
   t_job*.  POSIX jobspec syntax: %%, %+, % = current; %- = previous;
   %N = job number N; %string = job whose cmd starts with string.
   Empty spec also resolves to current.  Returns NULL on no match. */

#include "job_control.h"
#include "shell.h"
#include "libft.h"
#include <stdio.h>

/* Find the first job whose cmd prefix matches `str`.  Used for the
   `%string` jobspec form (e.g. `fg %vim` brings back vim). */
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

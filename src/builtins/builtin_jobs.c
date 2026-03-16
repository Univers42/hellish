/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_jobs.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"

static void	parse_jobs_flags(char **av, int ac, bool *show_pid, bool *long_fmt)
{
	int	i;

	*show_pid = false;
	*long_fmt = false;
	i = 1;
	while (i < ac && av[i][0] == '-')
	{
		if (ft_strchr(av[i], 'l'))
			*long_fmt = true;
		if (ft_strchr(av[i], 'p'))
			*show_pid = true;
		i++;
	}
}

int	builtin_jobs(t_shell *state, t_vec argv)
{
	t_job_table	*jt;
	bool		show_pid;
	bool		long_fmt;
	int			i;

	jt = &state->job_table;
	parse_jobs_flags((char **)argv.ctx, (int)argv.len, &show_pid, &long_fmt);
	job_update_status(jt);
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid)
		{
			if (show_pid)
				ft_printf("%d\n", jt->jobs[i].pgid);
			else
				job_print(&jt->jobs[i], jt->current,
					jt->previous, long_fmt);
		}
		i++;
	}
	return (0);
}

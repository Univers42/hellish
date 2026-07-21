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

/* Scan all option words for -l (long format) and -p (PIDs only). We use
   ft_strchr so combined flags like -lp work without writing two branches. */
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

/* jobs [-l] [-p]: list background (and stopped) jobs. We update statuses
   first via job_update_status (which reaps any that have finished since the
   last check) so the listing is accurate. -p prints only the process-group
   ID, useful for `kill $(jobs -p)`. -l adds the PID column. Listing a Done
   job counts as reporting it: the notified flag hides it from the next
   listing, but — matching bash, verified empirically — the entry itself
   stays so a later `wait $!` can still recover its exit status. Only wait
   purges for real. */
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
		if (jt->jobs[i].pgid && !(jt->jobs[i].status == JOB_DONE
				&& jt->jobs[i].notified))
		{
			if (show_pid)
				ft_printf("%d\n", jt->jobs[i].pgid);
			else
				job_print(&jt->jobs[i], jt->current,
					jt->previous, long_fmt);
			if (jt->jobs[i].status == JOB_DONE)
				jt->jobs[i].notified = true;
		}
		i++;
	}
	return (0);
}

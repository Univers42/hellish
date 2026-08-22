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

/* A finished job that has just been listed is over as far as the table is
   concerned: park its exit status in the bg_done ring so `wait` can still
   answer for it, then drop the entry so its NUMBER goes back into use. */
static void	retire_reported(t_shell *state, t_job *job)
{
	job->notified = true;
	bg_done_record(state, job->pgid, job->raw_status);
	job_remove(&state->job_table, job->id);
}

/* jobs [-l] [-p]: list background (and stopped) jobs. We update statuses
   first via job_update_status (which reaps any that have finished since the
   last check) so the listing is accurate. -p prints only the process-group
   ID, useful for `kill $(jobs -p)`. -l adds the PID column.

   Listing a finished job's STATUS retires it, and that is what frees its
   number:
   bash is back at [1] for the next job where hellish used to climb to [2].
   The comment that stood here claimed bash keeps the entry so a later
   `wait $!` can still answer -- half right. bash keeps the STATUS, not the
   job; retire_reported hands the raw waitpid word to the bg_done ring on
   the way out, which is the same trick and is why dropping the entry is
   safe.

   -p is the exception: it prints pgids, never a status, so bash does not
   count it as having reported anything and the job stays.

   We walk job NUMBERS (job_next_after) rather than table slots: a reaped
   job frees its slot, a later job reuses that slot, and slot order then
   prints [1] [4] [3] where bash prints [1] [3] [4]. */
int	builtin_jobs(t_shell *state, t_vec argv)
{
	t_job_table	*jt;
	t_job		*job;
	bool		show_pid;
	bool		long_fmt;
	int			i;

	jt = &state->job_table;
	parse_jobs_flags((char **)argv.ctx, (int)argv.len, &show_pid, &long_fmt);
	job_update_status(state);
	i = job_next_after(jt, 0);
	while (i)
	{
		job = job_find_id(jt, i);
		i = job_next_after(jt, i);
		if (job_finished(job) && job->notified)
			continue ;
		if (show_pid)
			ft_printf("%d\n", job->pgid);
		else
			job_print(job, jt->current, jt->previous, long_fmt);
		if (job_finished(job) && !show_pid)
			retire_reported(state, job);
	}
	return (0);
}

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
   answer for it, and mark it reported. The entry itself is dropped only
   once the WHOLE listing is out (job_purge_reported below): removing it
   here re-elected the current job mid-listing, and the next line came
   out as "[2]   Done" where bash prints "[2]+  Done" -- bash prints the
   batch with the markers as they stood, then deletes. */
static void	retire_reported(t_shell *state, t_job *job)
{
	job->notified = true;
	bg_done_record(state, job->pgid, job->raw_status);
}

/* One line of the listing: the pgid alone under -p, else the full row;
   a finished job is marked reported on the way out (retired after the
   whole listing, see retire_reported) -- except inside a $( ) body, which
   reports nothing on the shell's behalf: `$(jobs)$(jobs)` prints a's Done
   line twice in bash, and the shell's own `jobs` still prints it after. */
static void	list_one(t_shell *state, t_job *job, bool show_pid, bool long_fmt)
{
	t_job_table	*jt;

	jt = &state->job_table;
	if (show_pid)
		ft_printf("%d\n", job->pgid);
	else
		job_print(job, jt->current, jt->previous, long_fmt);
	if (job_finished(job) && !show_pid && !state->csf_depth)
		retire_reported(state, job);
}

/* Is this finished job one that a $( ) body must not list?
**
** bash reclaims a dead job at the next fork (cleanup_dead_jobs), and
** spares exactly one: the job $! names (last_asynchronous_pid). A $( )
** body therefore sees the parent's table minus the dead jobs that are no
** longer $!. Both halves matter, and hellish had neither right:
** `a & wait; $(jobs)` must still print a's Done line -- a is still $!,
** and hiding every finished job lost it -- while `a & wait; b & $(jobs)`
** must not, because b took $! over. The forked half hid nothing, so
** `a & $(jobs | wc -l); wait; b & $(jobs | wc -l)` counted 2 where bash
** counts 1 -- but only on a machine slow enough for a to die before the
** `wait` reached it, which is why it read as a CI flake and passed on
** every developer box. */
static bool	hidden_in_cmdsub(t_shell *state, t_job *job)
{
	if (!state->csf_depth)
		return (false);
	if (!state->last_bg_pid)
		return (true);
	return (job->pgid != (pid_t)ft_atoi(state->last_bg_pid));
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

   Inside a $( ) body -- run in-process or in the fork, both raise
   csf_depth -- `jobs` lists what bash's does there: the jobs still alive
   plus the one dead job $! still names (hidden_in_cmdsub), with the rest
   left for the shell itself to report. Nothing is retired or purged.

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
		if (!(job_finished(job)
				&& (job->notified || hidden_in_cmdsub(state, job))))
			list_one(state, job, show_pid, long_fmt);
	}
	if (!show_pid && !state->csf_depth)
		job_purge_reported(jt);
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_id.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "job_control.h"
#include "libft.h"

/* Pick the number the next job gets.

   bash's rule, measured against 5.3.9 rather than guessed: the new job takes
   (highest job number still in the table) + 1.  Two consequences fall out of
   that one sentence, and both were verified:

     - an EMPTY table restarts at [1].  A script that starts one background
       job, waits for it, and starts another gets [1] twice -- which is what
       `%1`, `fg %1` and `kill %1` in a bash-written script assume (issue #18).
     - a gap left by a reaped MIDDLE job is not filled.  With [1] and [3] live
       and [2] reaped, the next job is [4], not [2].

   "Still in the table" is the load-bearing part: a finished job holds its
   number until something reports it (`jobs`/`wait` -> job_purge_done, or the
   interactive prompt -> job_notify).  Allocating against live slots rather
   than a monotonic counter is what makes the number free itself. */
int	job_next_id(t_job_table *jt)
{
	int	i;
	int	hi;

	hi = 0;
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].id > hi)
			hi = jt->jobs[i].id;
		i++;
	}
	return (hi + 1);
}

/* Highest live job number other than `except` (-1 for "no exception"), or
   -1 when the table holds nothing else.  Job numbers are handed out in
   increasing order, so "highest" is also "most recent" -- which is the
   ordering bash's '+' and '-' markers actually follow. */
int	job_highest_id(t_job_table *jt, int except)
{
	int	i;
	int	best;

	best = -1;
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].id != except
			&& jt->jobs[i].id > best)
			best = jt->jobs[i].id;
		i++;
	}
	return (best);
}

/* Re-elect the '+' and '-' jobs after `gone` left the table.

   bash always marks the two most recent jobs; when one of them is reaped the
   marks move down.  hellish used to leave current/previous pointing at a job
   that no longer existed, so `sleep 9 & sleep 0 & wait %2; jobs` printed the
   survivor with NO marker where bash prints '+'.  Called from job_remove so
   every removal path -- wait, jobs, the interactive notifier -- gets it. */
void	job_reelect(t_job_table *jt, int gone)
{
	if (jt->current == gone)
	{
		jt->current = jt->previous;
		jt->previous = -1;
	}
	else if (jt->previous == gone)
		jt->previous = -1;
	if (!job_find_id(jt, jt->current))
		jt->current = job_highest_id(jt, -1);
	if (!job_find_id(jt, jt->previous) || jt->previous == jt->current)
		jt->previous = job_highest_id(jt, jt->current);
}

/* Smallest live job number strictly greater than `after`, or 0 when there is
   none.  `jobs` walks the table with this so the listing comes out in job
   NUMBER order; iterating raw slots printed [1] [4] [3] once a reaped job
   freed a slot in the middle and a later job reused it. */
int	job_next_after(t_job_table *jt, int after)
{
	int	i;
	int	best;

	best = 0;
	i = 0;
	while (i < JOB_MAX)
	{
		if (jt->jobs[i].pgid && jt->jobs[i].id > after
			&& (best == 0 || jt->jobs[i].id < best))
			best = jt->jobs[i].id;
		i++;
	}
	return (best);
}

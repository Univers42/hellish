/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_signal2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The asynchronous half of issue #17: the unsolicited stderr line a SCRIPT
   gets when a background job turns out to have been killed by a signal.
   Split from job_signal.c only because the norm caps a file at 5 functions. */

#include "job_control.h"
#include "shell.h"
#include "sh_input.h"
#include "libft.h"
#include <unistd.h>
#include <signal.h>

/* Signals bash refuses to announce.  Measured against bash 5.3.9, not
   guessed, because the set is not the obvious one: SIGTERM is silent even
   though `jobs` will happily print "Terminated" for the same job later.
   The through-line is deliberateness -- ^C, a quit, a closed pipe and a
   polite terminate are all things somebody MEANT, so bash does not
   interrupt the script's output to mention them; a SIGKILL, a segfault or
   an OOM kill is news. */
static bool	sig_worth_announcing(int sig)
{
	return (sig != SIGINT && sig != SIGQUIT
		&& sig != SIGPIPE && sig != SIGTERM);
}

/* The unsolicited stderr line bash writes when it NOTICES a background job
   died from a signal (issue #17) -- "script: line 5: 1234 Killed  sleep 9".

   Three conditions, all of them load-bearing:

   - Script and piped input only.  bash says nothing under -c, and the whole
     golden suite runs through -c, so announcing there would put a line in
     front of ~3000 diffs.  Interactive shells have their own voice already
     (job_notify prints the [1]+ form at the next prompt).
   - The original process only.  A forked child inherits a COPY of the job
     table, and without the pid check every subshell would re-announce its
     parent's dead jobs.
   - Announcing retires the job, exactly as bash does: after the message a
     later `jobs` shows nothing.  A suppressed signal is left in the table
     instead, which is why `jobs` can still report "Terminated" afterwards. */
void	job_notify_async(t_shell *state)
{
	t_job_table	*jt;
	t_job		*job;
	int			i;

	jt = &state->job_table;
	if (jt->count == 0 || (state->metinp != INP_FILE
			&& state->metinp != INP_NOTTY))
		return ;
	if (!state->pid || ft_atoi(state->pid) != (int)getpid())
		return ;
	job_update_status(state);
	i = job_next_after(jt, 0);
	while (i)
	{
		job = job_find_id(jt, i);
		i = job_next_after(jt, i);
		if (job->status == JOB_KILLED && !job->notified
			&& sig_worth_announcing(job->term_sig))
		{
			ft_eprintf("%s: %d %-27s%s%s\n", state->ctx, job->pgid,
				job_status_desc(job), job_core_suffix(job), job->cmd);
			job->notified = true;
			job_remove(jt, job->id);
		}
	}
}

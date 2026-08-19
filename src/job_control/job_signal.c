/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_signal.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Where a background job's DEATH BY SIGNAL is recorded and reported.

   bash reports a signalled job in two different voices and hellish had
   neither.  In `jobs` it replaces the "Done" column with the signal's own
   description -- "Killed", "Terminated", "Segmentation fault (core dumped)"
   -- and in a SCRIPT it also writes an unsolicited line to stderr the first
   time it notices, which is the whole of issue #17.  Both need the same two
   facts, so the job now remembers which signal killed it and whether that
   signal dumped core. */

#include "job_control.h"
#include "shell.h"
#include "sh_input.h"
#include "libft.h"
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

/* Decode one raw waitpid() status word into the job.  Two callers reach
   here -- the WNOHANG poll (job_update_status) and the ring that remembers
   a status for a later `wait` (bg_done_record) -- and they used to decode
   it separately, which is exactly how one of them could learn the signal
   and the other could not.  exit_code keeps bash's 128+signal convention
   either way; term_sig is what tells the display which voice to use.  The
   raw word is kept as well: retiring a job hands it to the bg_done ring so
   a later `wait` can still answer, which is the only reason `jobs` is
   allowed to drop the entry at all. */
void	job_record_exit(t_job *job, int status)
{
	job->raw_status = status;
	if (WIFSTOPPED(status))
		return ((void)(job->status = JOB_STOPPED));
	if (!WIFEXITED(status) && !WIFSIGNALED(status))
		return ;
	if (WIFEXITED(status))
	{
		job->status = JOB_DONE;
		job->exit_code = WEXITSTATUS(status);
		return ;
	}
	job->status = JOB_KILLED;
	job->term_sig = WTERMSIG(status);
	job->core_dumped = WCOREDUMP(status);
	job->exit_code = 128 + WTERMSIG(status);
}

/* "This job is over", whichever way it ended.  Every caller that used to
   test `status == JOB_DONE` means this: `wait` recovering a status, `jobs`
   deciding what to list, job_purge_done retiring reported entries.  Now
   that a signalled job stops at JOB_KILLED instead, testing JOB_DONE alone
   would strand it in the table forever. */
bool	job_finished(const t_job *job)
{
	return (job->status == JOB_DONE || job->status == JOB_KILLED);
}

/* The text that fills the status column.  For a signal death that is the
   system's own description, the same one bash prints, because bash asks the
   same libc -- which is what keeps the two agreeing on a platform whose
   wording differs (musl and glibc do not spell every signal alike).
   The result is BORROWED and read-only; it is char * rather than const
   char * only so the declaration aligns with the rest of the header. */
char	*job_status_desc(const t_job *job)
{
	if (job->status == JOB_RUNNING)
		return ((char *)"Running");
	if (job->status == JOB_STOPPED)
		return ((char *)"Stopped");
	if (job->status == JOB_DONE)
		return ((char *)"Done");
	if (job->status == JOB_KILLED && job->term_sig > 0)
		return (strsignal(job->term_sig));
	return ((char *)"Unknown");
}

/* bash appends this AFTER the padded status column, so the command text
   shifts right by 14 for a core-dumping signal and the status field width
   stays 27 either way (measured against bash 5.3.9, not guessed). */
char	*job_core_suffix(const t_job *job)
{
	if (job->status == JOB_KILLED && job->core_dumped)
		return ((char *)"(core dumped) ");
	return ((char *)"");
}

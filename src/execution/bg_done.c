/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bg_done.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "job_control.h"
#include <sys/wait.h>

/* Stash a finished background child's raw waitpid() status in the ring, so a
   later `wait <pid>` can report it. Writing into a ring means we always have a
   slot (the oldest unread entry is overwritten after BG_DONE_MAX unwaited
   children -- those were never waited for, so losing them is harmless).
   The job table entry (if this pid was a registered & job) flips to DONE
   here too, so the next `jobs` shows Done and can retire it. */
void	bg_done_record(t_shell *state, pid_t pid, int status)
{
	t_job	*job;

	state->bg_done[state->bg_done_next].pid = pid;
	state->bg_done[state->bg_done_next].status = status;
	state->bg_done_next = (state->bg_done_next + 1) % BG_DONE_MAX;
	job = job_find_pgid(&state->job_table, pid);
	if (job)
	{
		job->status = JOB_DONE;
		if (WIFEXITED(status))
			job->exit_code = WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			job->exit_code = 128 + WTERMSIG(status);
	}
}

/* Find and CONSUME a remembered status for `pid`. Returns 1 and fills *status
   when present (clearing the slot so a second wait on the same pid is a miss),
   0 otherwise. */
int	bg_done_take(t_shell *state, pid_t pid, int *status)
{
	int	i;

	i = 0;
	while (i < BG_DONE_MAX)
	{
		if (pid > 0 && state->bg_done[i].pid == pid)
		{
			*status = state->bg_done[i].status;
			state->bg_done[i].pid = 0;
			return (1);
		}
		i++;
	}
	return (0);
}

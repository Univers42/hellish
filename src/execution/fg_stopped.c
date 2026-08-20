/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fg_stopped.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/20 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include <sys/wait.h>
#include <unistd.h>

/* Label for a job that just stopped: the line the user typed, minus its
   trailing whitespace. bash shows the command text there, and the raw
   input buffer is the closest thing we have to it without rebuilding the
   command from the AST. */
static void	fg_stop_label(t_shell *st, char *buf, size_t size)
{
	size_t	n;
	char	*s;

	s = (char *)st->input.ctx;
	n = st->input.len;
	if (!s || !n)
		return ((void)ft_strlcpy(buf, "command", size));
	while (n && (s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
		n--;
	if (n >= size)
		n = size - 1;
	ft_memcpy(buf, s, n);
	buf[n] = '\0';
	if (!n)
		ft_strlcpy(buf, "command", size);
}

/* ^Z on a foreground command. The shell used to wait without WUNTRACED, so
   a stopped child never satisfied the wait and the REPL hung there for
   good: the tty went back to cooked mode, the kernel echoed whatever was
   typed next, and nothing ran it -- the shell looked alive and was not
   (issue #25). Now the wait sees the stop, we record the job so `jobs`
   lists it, announce it the way bash does, and hand a status back so the
   REPL draws another prompt.

   $? is 128 + the stopping signal, matching bash for a stopped foreground
   job. The leading newline puts the notice on its own line: the cursor is
   still sitting after the echoed ^Z when we get here.

   The job is filed under the child's REAL process group, not its pid.
   Foreground commands currently run inside the shell's own group, so those
   two differ, and `fg` drives the job by group -- kill(-pgid, SIGCONT) then
   waitpid(-pgid). Filing the pid would make both of those miss. */
void	fg_job_stopped(t_shell *st, t_execution_state *res, int status)
{
	t_job	*job;
	char	label[128];
	pid_t	pgid;

	fg_stop_label(st, label, sizeof(label));
	pgid = getpgid(res->pid);
	if (pgid <= 0)
		pgid = res->pid;
	job = job_add(&st->job_table, pgid, label, false);
	ft_printf("\n");
	if (job)
	{
		job_record_exit(job, status);
		job_print(job, st->job_table.current, st->job_table.previous, false);
	}
	res->status = 128 + WSTOPSIG(status);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_proc2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <sys/wait.h>
#include <sys/times.h>
#include <unistd.h>

/* Resolve one `wait` argument to a waitable pid. %spec goes through the
   job table (bash: unknown spec prints "no such job", rc 127). A plain
   argument must be a positive integer pid — anything else is bash's
   "not a pid or valid job spec", rc 1; a numeric non-child like 0 is a
   silent 127. Before this existed, `wait %1` fed atoi("%1") = 0 to
   waitpid and blocked on the whole process group forever. */
static pid_t	wait_arg_pid(t_shell *state, const char *arg, int *rc)
{
	t_job	*job;
	int		i;

	*rc = 127;
	if (arg[0] == '%')
	{
		job = job_by_spec(&state->job_table, arg);
		if (job)
			return (job->pgid);
		ft_eprintf("%s: wait: %s: no such job\n", state->ctx, arg);
		return (0);
	}
	i = 0;
	while (arg[i] >= '0' && arg[i] <= '9')
		i++;
	if (i == 0 || arg[i] != '\0')
	{
		ft_eprintf("%s: wait: `%s': not a pid or valid job spec\n",
			state->ctx, arg);
		*rc = 1;
		return (0);
	}
	return ((pid_t)ft_atoi(arg));
}

/* Wait for one explicit pid/jobspec: reap, record, purge — the same
   report-once lifecycle as the no-argument wait in builtin_wait. */
int	wait_one(t_shell *state, const char *arg)
{
	pid_t	pid;
	int		status;
	int		rc;
	int		drop;

	pid = wait_arg_pid(state, arg, &rc);
	if (pid <= 0)
		return (rc);
	if (waitpid(pid, &status, 0) < 0)
		return (reaped_job_status(state, pid));
	bg_done_record(state, pid, status);
	bg_done_take(state, pid, &drop);
	job_purge_done(&state->job_table);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (128 + WTERMSIG(status));
}

/* wait -n: wait for the NEXT single background child to finish and return
   its status (bash: 127 when there is nothing to wait for). waitpid(-1)
   reaps exactly one, which is recorded so a later `wait $!` on a
   different pid still resolves; the just-reaped one is purged. */
int	wait_n(t_shell *state)
{
	int		status;
	int		drop;
	pid_t	pid;

	pid = waitpid(-1, &status, 0);
	if (pid <= 0)
		return (127);
	bg_done_record(state, pid, status);
	bg_done_take(state, pid, &drop);
	job_purge_done(&state->job_table);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (128 + WTERMSIG(status));
}

/* times: print accumulated user/system CPU time for the shell and children.
   Formatted with libc snprintf: ft_printf lacks the %0Nld zero-padded
   variant and used to emit the raw format string here. */
int	builtin_times(t_shell *state, t_vec argv)
{
	struct tms	t;
	long		hz;
	char		line[256];

	(void)state;
	(void)argv;
	hz = sysconf(_SC_CLK_TCK);
	if (hz <= 0)
		hz = 100;
	times(&t);
	snprintf(line, sizeof(line), "%ldm%ld.%03lds %ldm%ld.%03lds\n",
		(t.tms_utime / hz) / 60, (t.tms_utime / hz) % 60,
		(t.tms_utime % hz) * 1000 / hz,
		(t.tms_stime / hz) / 60, (t.tms_stime / hz) % 60,
		(t.tms_stime % hz) * 1000 / hz);
	if (write(1, line, ft_strlen(line)) < 0)
		return (1);
	snprintf(line, sizeof(line), "%ldm%ld.%03lds %ldm%ld.%03lds\n",
		(t.tms_cutime / hz) / 60, (t.tms_cutime / hz) % 60,
		(t.tms_cutime % hz) * 1000 / hz,
		(t.tms_cstime / hz) / 60, (t.tms_cstime / hz) % 60,
		(t.tms_cstime % hz) * 1000 / hz);
	if (write(1, line, ft_strlen(line)) < 0)
		return (1);
	return (0);
}

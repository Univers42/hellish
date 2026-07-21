/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_proc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"
#include "env.h"
#include "job_control.h"
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <unistd.h>

void	procsub_detach_all(t_shell *state);

/* Build a NULL-terminated argv array for execve from argv[1..] (skipping
   argv[0] which is the "exec" word itself). We calloc so the sentinel NULL
   is already in place without an explicit assignment. The strings are NOT
   duplicated — execve does not need them to outlive the process image. */
static char	**dup_exec_argv(t_vec argv)
{
	char	**out;
	size_t	i;

	out = ft_calloc(argv.len, sizeof(char *));
	if (!out)
		return (NULL);
	i = 1;
	while (i < argv.len)
	{
		out[i - 1] = ((char **)argv.ctx)[i];
		i++;
	}
	return (out);
}

/* exec [command [args]]: replace the shell with command (or, with no command,
   leave the already-applied redirections in place). */
int	builtin_exec(t_shell *state, t_vec argv)
{
	char	*path;
	char	**xargv;
	char	**envp;

	if (argv.len < 2)
		return (procsub_detach_all(state), 0);
	if (find_cmd_path(state, ((char **)argv.ctx)[1], &path) != 0)
	{
		ft_eprintf("%s: exec: %s: not found\n", state->ctx,
			((char **)argv.ctx)[1]);
		exit(127);
	}
	xargv = dup_exec_argv(argv);
	envp = get_envp(state, path);
	execve(path, xargv, envp);
	ft_eprintf("%s: exec: %s: %s\n", state->ctx, path, strerror(errno));
	exit(126);
}

/* waitpid() failed (ECHILD): the child was already reaped by
   reap_background_children's WNOHANG poll between list items, or by
   job_update_status's poll when `jobs` listed it. bash remembers a
   finished job's status until `wait` collects it, so recover it from the
   bg_done ring first, then from the job table (which job_update_status
   fills but the ring never saw); only a genuinely unknown pid gives 127. */
static int	reaped_job_status(t_shell *state, pid_t pid)
{
	int		status;
	int		code;
	t_job	*job;

	job = job_find_pgid(&state->job_table, pid);
	if (bg_done_take(state, pid, &status))
	{
		if (job)
			job_remove(&state->job_table, job->id);
		if (WIFEXITED(status))
			return (WEXITSTATUS(status));
		if (WIFSIGNALED(status))
			return (128 + WTERMSIG(status));
		return (127);
	}
	if (job && job->status == JOB_DONE)
	{
		code = job->exit_code;
		job_remove(&state->job_table, job->id);
		return (code);
	}
	return (127);
}

/* wait [pid]: wait for background children (all of them if no pid given).
   Reaps route through bg_done_record so the job table flips to Done, then
   job_purge_done retires what was just reported — after `wait`, bash's
   `jobs` shows nothing, and ours must not either. The bg_done_take right
   after each record erases the ring's memory of that status: it was just
   reported, and bash answers a re-wait of the same pid with 127. */
int	builtin_wait(t_shell *state, t_vec argv)
{
	int		status;
	int		drop;
	pid_t	pid;

	status = 0;
	if (argv.len >= 2)
	{
		pid = (pid_t)ft_atoi(((char **)argv.ctx)[1]);
		if (waitpid(pid, &status, 0) < 0)
			return (reaped_job_status(state, pid));
		bg_done_record(state, pid, status);
		bg_done_take(state, pid, &drop);
		job_purge_done(&state->job_table);
		if (WIFEXITED(status))
			return (WEXITSTATUS(status));
		return (128 + WTERMSIG(status));
	}
	pid = waitpid(-1, &status, 0);
	while (pid > 0)
	{
		bg_done_record(state, pid, status);
		bg_done_take(state, pid, &drop);
		pid = waitpid(-1, &status, 0);
	}
	return (job_purge_done(&state->job_table), 0);
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

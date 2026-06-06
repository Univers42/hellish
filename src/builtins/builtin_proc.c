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
   reap_background_children's WNOHANG poll between list items. bash remembers a
   finished job's status until `wait` collects it, so recover the status we
   stashed at reap time; only a genuinely unknown pid yields 127. */
static int	reaped_job_status(t_shell *state, pid_t pid)
{
	int	status;

	if (bg_done_take(state, pid, &status))
	{
		if (WIFEXITED(status))
			return (WEXITSTATUS(status));
		if (WIFSIGNALED(status))
			return (128 + WTERMSIG(status));
	}
	return (127);
}

/* wait [pid]: wait for background children (all of them if no pid given). */
int	builtin_wait(t_shell *state, t_vec argv)
{
	int		status;
	pid_t	pid;

	status = 0;
	if (argv.len >= 2)
	{
		pid = (pid_t)ft_atoi(((char **)argv.ctx)[1]);
		if (waitpid(pid, &status, 0) < 0)
			return (reaped_job_status(state, pid));
		if (WIFEXITED(status))
			return (WEXITSTATUS(status));
		return (128 + WTERMSIG(status));
	}
	while (waitpid(-1, &status, 0) > 0)
		;
	(void)state;
	return (0);
}

/* times: print accumulated user/system CPU time for the shell and children. */
int	builtin_times(t_shell *state, t_vec argv)
{
	struct tms	t;
	long		hz;

	(void)state;
	(void)argv;
	hz = sysconf(_SC_CLK_TCK);
	if (hz <= 0)
		hz = 100;
	times(&t);
	ft_printf("%ldm%ld.%03lds %ldm%ld.%03lds\n",
		(t.tms_utime / hz) / 60, (t.tms_utime / hz) % 60,
		(t.tms_utime % hz) * 1000 / hz,
		(t.tms_stime / hz) / 60, (t.tms_stime / hz) % 60,
		(t.tms_stime % hz) * 1000 / hz);
	ft_printf("%ldm%ld.%03lds %ldm%ld.%03lds\n",
		(t.tms_cutime / hz) / 60, (t.tms_cutime / hz) % 60,
		(t.tms_cutime % hz) * 1000 / hz,
		(t.tms_cstime / hz) / 60, (t.tms_cstime / hz) % 60,
		(t.tms_cstime % hz) * 1000 / hz);
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   capture_subshell_output.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:35:39 by marvin            #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "executor.h"
#include "sys.h"

/* Fork a child for $(...) command substitution.  The child inherits the full
   shell state (so shell functions, local variables, $$ all work), connects
   its stdout to pipefd[1], then runs the command string via exec_string.
   "In-process" means: no execve of a fresh shell binary — the clone of the
   shell state IS the subshell.  The EXIT trap is cleared so it cannot fire
   inside the substitution. */
static pid_t	fork_and_run_inproc(t_shell *state, int pipefd[2],
					const char *cmd)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		xfree(state->traps[0]);
		state->traps[0] = NULL;
		pseudo_traps_quiet(state);
		state->cmdsub_in_place = cs_single_cmd(state, cmd);
		exit(exec_string(state, (char *)cmd) & 0xFF);
	}
	return (pid);
}

static void	update_cmdsub_status(t_shell *state, int status)
{
	if (WIFEXITED(status))
		state->last_cmdsub_status = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		state->last_cmdsub_status = 128 + WTERMSIG(status);
}

static char	*read_pipe_and_wait(t_shell *state, pid_t pid, int readfd)
{
	t_string	out;
	char		*ret;
	size_t		nlen;
	int			status;

	vec_init(&out);
	out.elem_size = 1;
	vec_append_fd(readfd, &out);
	close(readfd);
	status = 0;
	while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
		;
	update_cmdsub_status(state, status);
	ret = xmalloc(out.len + 1);
	if (!ret)
		return (xfree(out.ctx), ft_strdup(""));
	if (out.len)
		ft_memcpy(ret, out.ctx, out.len);
	ret[out.len] = '\0';
	nlen = out.len;
	while (nlen > 0 && ret[nlen - 1] == '\n')
		nlen--;
	ret[nlen] = '\0';
	xfree(out.ctx);
	return (ret);
}

/* Top-level $(...) capture: pipe + fork + drain + wait.  The classic
   trailing-newline strip mandated by POSIX is done by walking nlen back
   from the end of the buffer.  The exit status is saved in
   state->last_cmdsub_status so callers like $? pick it up correctly.
   Side-effect-free single-builtin bodies skip the fork entirely via
   cmdsub_fast (NULL means "not eligible, fork as usual").
   If the pipe() or fork() fails we return an empty string rather than
   crashing — worst case the command appears to produce no output. */
char	*capture_subshell_output(t_shell *state, const char *cmd)
{
	int		pipefd[2];
	pid_t	pid;
	char	*fast;

	if (is_empty_command(cmd))
		return (ft_strdup(""));
	fast = cmdsub_fast(state, cmd);
	if (fast)
		return (fast);
	if (pipe(pipefd) == -1)
		return (ft_strdup(""));
	pid = fork_and_run_inproc(state, pipefd, cmd);
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (ft_strdup(""));
	}
	close(pipefd[1]);
	return (read_pipe_and_wait(state, pid, pipefd[0]));
}

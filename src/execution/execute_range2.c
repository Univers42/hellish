/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_range2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/15 01:59:37 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 01:59:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Child body for a background command group (& operator).  We move into
   our own process group (setpgid) so job-control signals from the terminal
   don't reach us.  stdin is redirected from /dev/null (POSIX: background
   commands that try to read stdin get EOF, not a blocking read from the
   tty).  SIGINT/SIGQUIT/SIGTSTP/SIGTTIN/SIGTTOU are all ignored so the
   user can keep typing without accidentally killing the background job. */
static void	bg_child_body(t_shell *state, t_executable_node *exe,
				size_t start, size_t end)
{
	int					null_fd;
	t_execution_state	res;

	setpgid(0, 0);
	null_fd = open("/dev/null", O_RDONLY);
	if (null_fd >= 0)
	{
		dup2(null_fd, STDIN_FILENO);
		close(null_fd);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	res = execute_range(state, exe, start, end);
	exit(res.status);
}

/* Fork a background job, bump the bg counter, record $! (last_bg_pid),
   and print "[n] pid" if running interactively.  The parent always returns
   status 0 immediately (background jobs never block the parent).  We count
   jobs so reap_background_children knows whether to bother calling waitpid
   at all -- no jobs means no syscall overhead. */
t_execution_state	execute_range_background(t_shell *state,
										t_executable_node *exe,
										size_t start, size_t end)
{
	pid_t	pid;

	pid = fork();
	if (pid == 0)
		bg_child_body(state, exe, start, end);
	if (pid < 0)
		critical_error_errno_ctx("fork");
	procsub_close_fds_parent(state);
	state->bg_job_count++;
	xfree(state->last_bg_pid);
	state->last_bg_pid = ft_itoa(pid);
	if (state->metinp == INP_RL)
		ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return (res_status(0));
}

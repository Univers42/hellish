/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_fork.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* The forked line reader, kept for one release behind HELLISH_RL_FORK=1.
**
** Every line used to be read this way: fork, readline() in the child, the
** line back over a pipe. The child existed to contain readline's signal
** handlers and terminal state, and it cost a process per prompt. The line
** is now read in the shell itself (rl_inproc.c); this path stays so a
** regression there can be sidestepped on a user's machine without a
** reinstall, and goes away in 3.3.0.
**
** One trap it still has to respect: readline's buffer is libc-malloc'd, so
** free(ret) uses libc free, not xfree -- at SAFE=0 that would hit the
** ft_malloc heap and corrupt it. */

/* Child side: edit the line, send it, leave. Exit 0 = line, 1 = EOF. A
   widget's cd is reported beside the line (zle_cwd.c), since nothing else
   the child changes outlives it. */
static void	bg_readline(int outfd, char *prompt, t_shell *state)
{
	char	*ret;
	char	*row;

	row = rl_editor_enter(state, prompt);
	ret = readline(row);
	zle_cwd_send();
	if (!ret)
		(close(outfd), exit (1));
	(write_to_file(ret, outfd), free(ret), close(outfd), exit(0));
}

/* Parent side: drain the pipe into the buffer and wait for the child. If
   the child was killed by a signal (e.g. SIGINT in readline), return 2 to
   propagate the interrupt; otherwise use the child's exit status (0 =
   line, 1 = EOF). The waitpid loop retries on EINTR.
   The animation frames are single-shot: the fork that just happened gave
   the child its own armed copy, so the parent's cells are disarmed here.
   A continuation read (dquote> / heredoc> / loop body) forks again WITHOUT
   a ps1_animated render in between -- the rows above its cursor are the
   user's earlier input lines, and an armed hook would stamp the PS1 info
   row over them once per tick. Only the next PS1 render re-arms. */
static int	attach_input_readline(t_shell *state, int pp[2], int pid)
{
	int	status;

	anim_cells()->count = 0;
	close(pp[1]);
	vec_append_fd(pp[0], &state->rl.buff);
	buff_readline_update(&state->rl);
	close(pp[0]);
	zle_cwd_adopt(state);
	while (1)
		if (waitpid(pid, &status, 0) != -1)
			break ;
	if (WIFSIGNALED(status))
	{
		ft_eprintf("\n");
		return (2);
	}
	return (WEXITSTATUS(status));
}

/* The fork dance: create the pipe, fork, and hand each side to its
   function. The child never returns (bg_readline always exits);
   ft_assert(0) below is just a belt-and-suspenders guard in case the
   compiler does not see that. */
int	rl_read_fork(t_shell *state, char *prompt)
{
	int	pp[2];
	int	pid;

	if (pipe(pp))
		critical_error_errno_ctx("pipe");
	zle_cwd_open();
	pid = fork();
	if (pid == 0)
	{
		readline_bg_signals();
		close(pp[0]);
		bg_readline(pp[1], prompt, state);
	}
	else if (pid < 0)
		critical_error_errno_ctx("fork");
	else
		return (attach_input_readline(state, pp, pid));
	ft_assert(0);
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_range_bg.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 15:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 15:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* The signal disposition POSIX requires of an ASYNCHRONOUS child when job
   control is off: SIGINT and SIGQUIT ignored, so a ^C aimed at the
   foreground job -- or an explicit `kill -INT "$!"` -- does not take the
   background command down with it.  bash does the same, which is why
   `sleep 3 & kill -INT $!; wait $!` returns 0 there and not 130.

   This has to be re-applied after default_signal_handlers() in the
   exec-in-place path (that call resets everything to SIG_DFL, which is
   right for a $( ) body and wrong for an async one). */
void	async_child_signals(void)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
}

/* Child body for a background command group (& operator).  We move into
   our own process group (setpgid) so job-control signals from the terminal
   don't reach us.  stdin is redirected from /dev/null (POSIX: background
   commands that try to read stdin get EOF, not a blocking read from the
   tty).  POSIX also resets the parent's traps to default on entry to an
   async child (reset_traps_child) -- a signal aimed at this child must not
   run the parent's handler string, and the parent's EXIT trap is not ours
   to fire.  The reset comes first so the explicit SIG_IGNs below still win:
   SIGINT/SIGQUIT (POSIX) plus SIGTSTP/SIGTTIN/SIGTTOU are all ignored so the
   user can keep typing without accidentally killing the background job. */

/* The one AST shape a background child may execve in place: the range is a
   single pipeline, that pipeline is a single command, and that command is a
   lone AST_SIMPLE_COMMAND.  Anything else -- a real pipeline, a subshell, a
   brace group, if/for/while, `!` negation, or several &&-joined commands --
   still has work to do after the command returns, and exec is irreversible,
   so the scan errs toward the extra fork exactly like cs_single_cmd does.

   The negate check is not paranoia: `! cmd &` must invert the status, which
   only happens after the command returns.

   A builtin or function needs no permission from us -- dispatch_cmd routes
   those away from execute_cmd_bg before the node is ever consulted. */
static t_ast_node	*bg_lone_command(t_ast_node *list, size_t start,
						size_t end)
{
	t_ast_node	*pipe;
	t_ast_node	*cmd;

	if (end - start != 1)
		return (NULL);
	pipe = &((t_ast_node *)list->children.ctx)[start];
	if (pipe->node_type != AST_COMMAND_PIPELINE || pipe->children.len != 1
		|| pipe->negate)
		return (NULL);
	cmd = &((t_ast_node *)pipe->children.ctx)[0];
	if (cmd->node_type != AST_COMMAND || cmd->children.len != 1)
		return (NULL);
	cmd = &((t_ast_node *)cmd->children.ctx)[0];
	if (cmd->node_type != AST_SIMPLE_COMMAND)
		return (NULL);
	return (cmd);
}

static void	bg_child_body(t_shell *state, t_executable_node *exe,
				size_t start, size_t end)
{
	int					null_fd;
	t_execution_state	res;

	setpgid(0, 0);
	state->bg_exec_node = bg_lone_command(exe->node, start, end);
	null_fd = open("/dev/null", O_RDONLY);
	if (null_fd >= 0)
	{
		dup2(null_fd, STDIN_FILENO);
		close(null_fd);
	}
	reset_traps_child(state);
	async_child_signals();
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	res = execute_range(state, exe, start, end);
	exit(res.status);
}

/* Build a short human label for the jobs table from the range's source
   text. Token slices point into the input buffer, so the label is a slice
   of what the user typed rather than a reconstruction.

   Descending to the first token is not enough on its own: a compound has
   no token of its own, so the walk lands on the first token INSIDE it and
   the opening delimiter is lost -- `( sleep 1 ) &` was labelled
   "sleep 1 )" and `for i in 1; do ... done &` became "i in 1; do ...".
   So once the leftmost token is found we walk BACK to the byte after the
   nearest command separator, which recovers the `(`, `{`, `for`, `if` or
   `while` that started it. The backward scan only runs when the token is
   known to point inside state->input, so it can never wander off a
   heredoc, function-body or eval buffer.

   The forward scan ends at the `&` that backgrounds the job, but has to
   step over a `&&` -- stopping at its first byte turned
   `echo a && sleep 1 &` into the label "echo a". */
static void	bg_job_label(t_shell *state, t_ast_node *node, char *buf,
			size_t n)
{
	const char	*base;
	const char	*p;
	size_t		len;

	while (node && !node->token.start && node->children.len > 0)
		node = (t_ast_node *)node->children.ctx;
	if (!node || !node->token.start)
		return ((void)ft_strlcpy(buf, "job", n));
	p = node->token.start;
	base = (const char *)state->input.ctx;
	if (base && p >= base && p < base + state->input.len)
	{
		while (p > base && !ft_strchr(";&|\n", p[-1]))
			p--;
		while (*p == ' ' || *p == '\t')
			p++;
	}
	len = 0;
	while (p[len] && p[len] != '\n' && len < n - 1
		&& (p[len] != '&' || p[len + 1] == '&'))
		len += 1 + (p[len] == '&');
	while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t'))
		len--;
	ft_memcpy(buf, p, len);
	buf[len] = '\0';
}

/* Fork a background job, register it in the job table (this is what makes
   jobs/fg/bg/kill %1 see it), record $! (last_bg_pid), and print
   "[id] pid" if running interactively.  The parent always returns status
   0 immediately (background jobs never block the parent).  bg_job_count
   still gates reap_background_children's waitpid call. */
t_execution_state	execute_range_background(t_shell *state,
										t_executable_node *exe,
										size_t start, size_t end)
{
	pid_t	pid;
	t_job	*job;
	char	label[64];

	pid = fork();
	if (pid == 0)
		bg_child_body(state, exe, start, end);
	if (pid < 0)
		critical_error_errno_ctx("fork");
	procsub_close_fds_parent(state);
	state->bg_job_count++;
	xfree(state->last_bg_pid);
	state->last_bg_pid = ft_itoa(pid);
	bg_job_label(state, (t_ast_node *)exe->node->children.ctx + start,
		label, sizeof(label));
	job = job_add(&state->job_table, pid, label, true);
	if (state->metinp == INP_RL && job)
		ft_printf("[%d] %d\n", job->id, pid);
	else if (state->metinp == INP_RL)
		ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return (res_status(0));
}

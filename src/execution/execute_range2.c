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
#include "ft_builtins.h"

void	exit_clean(t_shell *state, int code);

/* Child body for a background command group (& operator).  We move into
   our own process group (setpgid) so job-control signals from the terminal
   don't reach us.  stdin is redirected from /dev/null (POSIX: background
   commands that try to read stdin get EOF, not a blocking read from the
   tty).  POSIX also resets the parent's traps to default on entry to an
   async child (reset_traps_child) -- a signal aimed at this child must not
   run the parent's handler string, and the parent's EXIT trap is not ours
   to fire.  The reset comes first so the explicit SIG_IGNs below still win:
   SIGINT/SIGQUIT/SIGTSTP/SIGTTIN/SIGTTOU are all ignored so the user can
   keep typing without accidentally killing the background job. */
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
	reset_traps_child(state);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	res = execute_range(state, exe, start, end);
	exit(res.status);
}

/* Build a short human label for the jobs table from the range's first
   simple command: walk down to the first node carrying a source token
   and take up to 48 bytes from its start — token slices are contiguous
   source text, so this shows "sleep 5" for `sleep 5 &` without any
   reconstruction machinery. Falls back to "job" for tokenless trees. */
static void	bg_job_label(t_ast_node *node, char *buf, size_t n)
{
	size_t	len;

	while (node && !node->token.start && node->children.len > 0)
		node = (t_ast_node *)node->children.ctx;
	if (!node || !node->token.start)
		return ((void)ft_strlcpy(buf, "job", n));
	len = 0;
	while (node->token.start[len] && node->token.start[len] != '\n'
		&& node->token.start[len] != '&' && len < n - 1)
		len++;
	while (len > 0 && (node->token.start[len - 1] == ' '
			|| node->token.start[len - 1] == '\t'))
		len--;
	ft_memcpy(buf, node->token.start, len);
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
	bg_job_label((t_ast_node *)exe->node->children.ctx + start,
		label, sizeof(label));
	job = job_add(&state->job_table, pid, label, true);
	if (state->metinp == INP_RL && job)
		ft_printf("[%d] %d\n", job->id, pid);
	else if (state->metinp == INP_RL)
		ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return (res_status(0));
}

/* Decide whether to run the next command based on the operator that
   separates it from the previous one.  ; and newline always run.  &&
   short-circuits on failure; || short-circuits on success.  Ctrl-C
   (ctrl_c=true) stops execution regardless of operator so an interrupted
   pipeline does not stumble forward into the right-hand side. */
bool	should_execute(t_execution_state prev_status, t_tt prev_op)
{
	if (prev_status.ctrl_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR || prev_op == TT_AMPERSAND);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AMPERSAND)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

/* set -e: exit when the AND-OR list's last *executed* command failed. This
   excludes the left operand of && / || (it isn't the one that ran last, e.g.
   `false && true` skips true so `false` is non-terminal), a negated pipeline
   (! ...), and any list run as an if/while/until condition (errexit_off). */
void	errexit_check(t_shell *state, t_execution_state st,
			bool ran, t_ast_node *last)
{
	if (state->errexit_off || !ran || st.status == 0)
		return ;
	if (last && last->node_type == AST_COMMAND_PIPELINE && last->negate)
		return ;
	if (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind)
		return ;
	fire_err_trap(state, st.status);
	if (state->opt_errexit)
		exit_clean(state, st.status);
}

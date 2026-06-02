/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_range.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 16:26:46 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 01:59:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

void	exit_clean(t_shell *state, int code);

/* set -e: exit when the AND-OR list's last *executed* command failed. This
   excludes the left operand of && / || (it isn't the one that ran last, e.g.
   `false && true` skips true so `false` is non-terminal), a negated pipeline
   (! ...), and any list run as an if/while/until condition (errexit_off). */
static void	errexit_check(t_shell *state, t_execution_state st,
				bool ran, t_ast_node *last)
{
	if (!state->opt_errexit || state->errexit_off || !ran || st.status == 0)
		return ;
	if (last && last->node_type == AST_COMMAND_PIPELINE && last->negate)
		return ;
	if (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind)
		return ;
	exit_clean(state, st.status);
}

static void	execute_then(t_executable_node *exe,
							t_ast_node *child,
							t_shell *state,
							t_execution_state *status)
{
	t_executable_node	exe_curr;

	exe_curr = *exe;
	exe_curr.node = child;
	*status = execute_tree_node(state, &exe_curr);
	if (status->pid != -1)
		exe_res_set_status(status);
	ft_assert(status->status != -1);
}

/* Execute a range of chi{ldren [start, end)
as a sequence in the current process.
   This handles && and || chaining within the range. */
t_execution_state	execute_range(t_shell *state, t_executable_node *exe,
							size_t start, size_t end)
{
	t_execution_state	status;
	t_tt				op;
	size_t				i;
	t_ast_node			*child;
	bool				ran;

	status = res_status(0);
	op = TT_SEMICOLON;
	i = start;
	ran = false;
	child = NULL;
	while (i < end)
	{
		child = &((t_ast_node *)exe->node->children.ctx)[i];
		if (child->node_type == AST_TOKEN)
		{
			op = child->token.tt;
			i++;
			continue ;
		}
		ran = should_execute(status, op);
		if (ran)
			execute_then(exe, child, state, &status);
		i++;
	}
	errexit_check(state, status, ran, child);
	return (status);
}

/* Execute a command sequence in the background */
t_execution_state	execute_range_background(t_shell *state,
										t_executable_node *exe,
										size_t start, size_t end)
{
	pid_t				pid;
	int					null_fd;
	t_execution_state	res;

	pid = fork();
	if (pid == 0)
	{
		setpgid(0, 0);
		null_fd = open("/dev/null", O_RDONLY);
		if (null_fd >= 0)
			(dup2(null_fd, STDIN_FILENO), close(null_fd));
		(signal(SIGINT, SIG_IGN), signal(SIGQUIT, SIG_IGN));
		(signal(SIGTSTP, SIG_IGN), signal(SIGTTIN, SIG_IGN));
		signal(SIGTTOU, SIG_IGN);
		res = execute_range(state, exe, start, end);
		exit(res.status);
	}
	if (pid < 0)
		critical_error_errno_ctx("fork");
	procsub_close_fds_parent(state);
	state->bg_job_count++;
	free(state->last_bg_pid);
	state->last_bg_pid = ft_itoa(pid);
	if (state->metinp == INP_RL)
		ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return (res_status(0));
}

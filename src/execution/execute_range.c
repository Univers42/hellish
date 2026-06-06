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

/* Run one pipeline/command node from the sequence, inheriting the
   surrounding exe's fds and redirects (via struct copy).  We immediately
   wait for any forked child via exe_res_set_status so the status is known
   before the next && / || decision.  ft_assert(status != -1) confirms we
   did not accidentally leave a zombie unwaited. */
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

/* Execute the range of AST children [start, end) as a sequential
   AND-OR list.  AST_TOKEN children carry the operator (&&, ||, ;, etc.)
   and shift the `op` variable that should_execute reads.  Non-token
   children are command nodes; we run them if should_execute says yes.
   errexit_check fires after the last command in the range, not after each
   one, to match POSIX behaviour for AND-OR lists under set -e. */
t_execution_state	execute_range(t_shell *state, t_executable_node *exe,
							size_t start, size_t end)
{
	t_execution_state	status;
	t_tt				op;
	t_ast_node			*child;
	size_t				i;
	bool				ran;

	status = res_status(0);
	op = TT_SEMICOLON;
	i = start;
	ran = false;
	child = NULL;
	while (i < end)
	{
		child = &((t_ast_node *)exe->node->children.ctx)[i++];
		if (child->node_type == AST_TOKEN)
		{
			op = child->token.tt;
			continue ;
		}
		ran = should_execute(status, op);
		if (ran)
			execute_then(exe, child, state, &status);
	}
	errexit_check(state, status, ran, child);
	return (status);
}

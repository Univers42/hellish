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

void	errexit_check(t_shell *state, t_execution_state st, bool ran,
			t_ast_node *last);

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
		exe_res_set_status(state, status);
	ft_assert(status->status != -1);
}

/* Is the very next child an && or || operator token?  If so, the command
   about to run is a non-final AND-OR operand: POSIX says set -e is
   ignored for it — INCLUDING everything it runs deeper down (a function
   body's own ranges see errexit_off and stay alive; `f() { false; };
   f || true` must reach the `true`). */
static bool	next_is_andor(t_ast_node *node, size_t i, size_t end)
{
	t_ast_node	*c;

	if (i >= end)
		return (false);
	c = &((t_ast_node *)node->children.ctx)[i];
	return (c->node_type == AST_TOKEN
		&& (c->token.tt == TT_AND || c->token.tt == TT_OR));
}

/* execute_then with errexit suppressed for the duration.  errexit_off is
   a DEPTH, not a flag: this used to save it into a bool and write back
   `true`, so an `A && B` inside an if-condition (depth 2) restored depth 1,
   the condition's own decrement then reached 0, and the next failing
   command in the same function exited a shell that was still inside a
   suppressed `f || ...` -- issue #121.  Nesting composes only if every
   site pairs ++ with --, the way run_condition and the loops already do. */
static void	execute_lhs(t_executable_node *exe, t_ast_node *child,
				t_shell *state, t_execution_state *status)
{
	state->errexit_off++;
	execute_then(exe, child, state, status);
	state->errexit_off--;
}

/* An AST_TOKEN child carries the list operator (&&, ||, ;, newline).
   It executes nothing itself: it only becomes the pending operator that
   should_execute consults for the next command child. */
static bool	take_operator(t_ast_node *child, t_tt *op)
{
	if (child->node_type != AST_TOKEN)
		return (false);
	*op = child->token.tt;
	return (true);
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
		if (take_operator(child, &op))
			continue ;
		ran = should_execute(status, op);
		if (ran && next_is_andor(exe->node, i, end))
			execute_lhs(exe, child, state, &status);
		else if (ran)
			execute_then(exe, child, state, &status);
	}
	errexit_check(state, status, ran, child);
	return (status);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_if.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Run one branch of an if/elif/else chain with the default fds (stdin=0,
   stdout=1) and modify_parent_ctx=true so builtins inside the branch
   affect the parent shell.  The child is an AST_COMPOUND_LIST or similar
   that execute_tree_node knows how to handle. */
static t_execution_state	run_child(t_shell *state, t_ast_node *child)
{
	t_executable_node	child_exe;

	child_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, child, true);
	return (execute_tree_node(state, &child_exe));
}

/* The if/elif condition is a list run with errexit suppressed (POSIX: -e is
   ignored for the compound list following if/elif/while/until). */
static t_execution_state	run_condition(t_shell *state, t_ast_node *child)
{
	t_execution_state	s;

	state->errexit_off++;
	s = run_child(state, child);
	state->errexit_off--;
	return (s);
}

/* if/elif/else executor.  The AST packs branches as flat children:
   [cond0, body0, cond1, body1, ..., else_body?].  An odd child count
   means an else clause is present.  We walk pairs: run the condition
   with errexit suppressed (POSIX says -e must not fire on the condition
   of if/elif/while/until), and on exit 0 run the matching body and
   return.  If we exhaust all conditions and there is a trailing else
   child, run that.  If nothing matched, return status 0 (POSIX). */
t_execution_state	execute_if(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	size_t				i;
	size_t				len;

	len = exe->node->children.len;
	i = 0;
	while (i + 1 < len)
	{
		status = run_condition(state, vec_idx(&exe->node->children, i));
		if (status.status == 0)
			return (run_child(state,
					vec_idx(&exe->node->children, i + 1)));
		i += 2;
	}
	if (i < len)
		return (run_child(state, vec_idx(&exe->node->children, i)));
	return (res_status(0));
}

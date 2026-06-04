/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_while.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Run a loop child (condition/body) IN PLACE. Word expansion is now
   non-destructive (expand_word_ro / clone-into-scratch), so `child` survives
   intact for the next iteration; the loop owns it and frees it once at the
   end. Mirrors execute_if.c run_child. */
static t_execution_state	run_node(t_shell *state, t_ast_node *child)
{
	t_executable_node	child_exe;

	child_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, child, true);
	return (execute_tree_node(state, &child_exe));
}

/* Consume a pending break/continue after a loop body. Returns 1 if the
   current loop must stop (break, continue targeting an outer loop, or
   exit/unwind), 0 to proceed to the next iteration. */
int	handle_loop_ctl(t_shell *state)
{
	if (state->loop_break)
	{
		state->loop_break--;
		return (1);
	}
	if (state->loop_continue)
	{
		state->loop_continue--;
		return (state->loop_continue != 0);
	}
	return (state->should_exit || state->func_return
		|| get_g_sig()->should_unwind);
}

/*
** AST_WHILE: children[0]=condition, children[1]=body
** While condition exits 0, execute body. Return last body status.
** AST_UNTIL: same but loop while condition exits non-zero.
*/
t_execution_state	execute_while(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	t_execution_state	body_status;
	bool				is_until;

	ft_assert(exe->node->children.len == 2);
	is_until = (exe->node->node_type == AST_UNTIL);
	body_status = res_status(0);
	state->loop_depth++;
	while (1)
	{
		state->errexit_off++;
		status = run_node(state, vec_idx(&exe->node->children, 0));
		state->errexit_off--;
		if ((!is_until && status.status != 0)
			|| (is_until && status.status == 0))
			break ;
		body_status = run_node(state, vec_idx(&exe->node->children, 1));
		if (handle_loop_ctl(state))
			break ;
	}
	state->loop_depth--;
	return (body_status);
}

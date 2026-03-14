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

static t_execution_state	run_node(t_shell *state, t_ast_node *child)
{
	t_executable_node	child_exe;
	t_ast_node			copy;
	t_execution_state	status;

	copy = clone_ast(child);
	child_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, &copy, false);
	status = execute_tree_node(state, &child_exe);
	free_ast(&copy);
	return (status);
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
	while (1)
	{
		status = run_node(state, vec_idx(&exe->node->children, 0));
		if ((!is_until && status.status != 0)
			|| (is_until && status.status == 0))
			break ;
		body_status = run_node(state, vec_idx(&exe->node->children, 1));
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
	}
	return (body_status);
}

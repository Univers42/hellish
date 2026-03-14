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

static t_execution_state	run_child(t_shell *state, t_ast_node *child)
{
	t_executable_node	child_exe;

	child_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, child, true);
	return (execute_tree_node(state, &child_exe));
}

/*
** AST_IF children: pairs of (condition, body)
** If total children is odd and > 1, last child is else-body.
** Execute conditions in pairs: if condition[i] succeeds (exit 0),
** execute body[i+1] and return. Otherwise try next pair (elif).
** If all conditions fail and else exists, execute it.
*/
t_execution_state	execute_if(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	size_t				i;
	size_t				len;

	len = exe->node->children.len;
	i = 0;
	while (i + 1 < len)
	{
		status = run_child(state, vec_idx(&exe->node->children, i));
		if (status.status == 0)
			return (run_child(state,
					vec_idx(&exe->node->children, i + 1)));
		i += 2;
	}
	if (i < len)
		return (run_child(state, vec_idx(&exe->node->children, i)));
	return (res_status(0));
}

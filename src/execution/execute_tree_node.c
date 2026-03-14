/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_tree_node.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:09:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 14:07:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);

/* dispatch to the right executor for each node */
t_execution_state	execute_tree_node(t_shell *state,
						t_executable_node *exe)
{
	t_execution_state		status;
	t_ast_type				t;

	t = exe->node->node_type;
	status = res_status(0);
	if (t == AST_COMMAND_PIPELINE)
		status = execute_pipeline(state, exe);
	else if (t == AST_SIMPLE_LIST || t == AST_COMPOUND_LIST)
		status = execute_simple_list(state, exe);
	else if (t == AST_IF)
		status = execute_if(state, exe);
	else if (t == AST_WHILE || t == AST_UNTIL)
		status = execute_while(state, exe);
	else if (t == AST_FOR)
		status = execute_for(state, exe);
	else
		ft_assert(0);
	set_cmd_status(state, status);
	return (status);
}

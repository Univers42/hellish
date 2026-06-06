/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_tree_node.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:09:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);
t_execution_state	execute_case(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe);

/* Single dispatch point for the AST executor.  Every recursive call
   from the specialised executors (execute_if, execute_while, ...) comes
   back here, so set_cmd_status is called consistently on every node --
   that keeps $? correct even for nested constructs.  The ft_assert(0)
   at the bottom is the "exhausted all known node types" guard: if the
   parser ever emits a type we forgot to handle, we crash loudly instead
   of silently returning 0 and hiding the bug. */
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
	else if (t == AST_CASE)
		status = execute_case(state, exe);
	else if (t == AST_FUNCTION_DEF)
		status = execute_func_def(state, exe);
	else
		ft_assert(0);
	set_cmd_status(state, status);
	return (status);
}

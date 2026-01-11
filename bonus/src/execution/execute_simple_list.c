/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

bool	should_execute(t_exe_res prev_status, t_tt prev_op)
{
	if (prev_status.c_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

t_exe_res	execute_simple_list(t_shell *state, t_executable_node *exe)
{
	t_exe_res			status;
	t_tt				op;
	t_executable_node	exe_curr;
	size_t				i;

	status = res_status(0);
	op = TT_SEMICOLON;
	i = 0;
	while (i < exe->node->children.len)
	{
		exe_curr = *exe;
		exe_curr.node = &((t_ast_node *)exe->node->children.ctx)[i];
		if (should_execute(status, op))
		{
			status = execute_tree_node(state, &exe_curr);
			ft_assert(status.status != -1);
		}
		i++;
		if (i < exe->node->children.len)
			op = ((t_ast_node *)exe->node->children.ctx)[i].token.tt;
		i++;
	}
	return status;
}

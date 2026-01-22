/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_command.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:41 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 15:08:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"


// gives back pid;
t_exe_res	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_node	*curr;
	size_t		i;
	int			redir_idx;

	ft_assert(exe->node->children.len >= 1);
	if (((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SIMPLE_COMMAND)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_simple_command(state, exe));
	}
	ft_assert(((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SUBSHELL);
	/* ensure redirs vector initialized */
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = vec_idx(&exe->node->children, i);
		ft_assert(curr->node_type == AST_REDIRECT);
		if (redirect_from_ast_redir(state, curr, &redir_idx))
			return (res_status(AMBIGUOUS_REDIRECT));
		{ int idx = redir_idx; vec_push(&exe->redirs, &idx); }
	}
	exe->node = vec_idx(&exe->node->children, 0);
	exe->modify_parent_context = true;
	return (execute_subshell(state, exe));
}
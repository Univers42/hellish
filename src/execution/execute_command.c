/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_command.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:41 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	ensure_redirs_initialized(t_executable_node *exe);
static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe);

t_execution_state	execute_command(t_shell *state, t_executable_node *exe)
{
	ft_assert(exe->node->children.len >= 1);
	if (((t_ast_node *)exe->node->children.ctx)[0].node_type
		== AST_SIMPLE_COMMAND)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_simple_command(state, exe));
	}
	ft_assert(((t_ast_node *)exe->node->children.ctx)[0].node_type
		== AST_SUBSHELL);
	ensure_redirs_initialized(exe);
	if (collect_redirects_from_ast(state, exe))
		return (res_status(AMBIGUOUS_REDIRECT));
	exe->node = vec_idx(&exe->node->children, 0);
	exe->modify_parent_ctx = true;
	return (execute_subshell(state, exe));
}

/* Ensure exe->redirs is initialized */
static void	ensure_redirs_initialized(t_executable_node *exe)
{
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
}

/*
** Collect redirect indices from exe->node children (starting at index 1)
** Returns 0 on success, AMBIGUOUS_REDIRECT if
redirect_from_ast_redir signals error.
*/
static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe)
{
	size_t		i;
	t_ast_node	*curr;
	int			redir_idx;

	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = vec_idx(&exe->node->children, i);
		ft_assert(curr->node_type == AST_REDIRECT);
		if (redirect_from_ast_redir(state, curr, &redir_idx))
			return (AMBIGUOUS_REDIRECT);
		vec_push_int(&exe->redirs, redir_idx);
	}
	return (0);
}

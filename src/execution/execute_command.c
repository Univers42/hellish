/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_command.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:41 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	ensure_redirs_initialized(t_executable_node *exe);
static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe);

t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe);

static bool	is_compound_ast(t_ast_type t)
{
	return (t == AST_IF || t == AST_WHILE || t == AST_UNTIL
		|| t == AST_FOR || t == AST_BRACE_GROUP);
}

static t_execution_state	dispatch_compound(t_shell *state,
								t_executable_node *exe)
{
	t_ast_type	t;

	t = exe->node->node_type;
	if (t == AST_IF)
		return (execute_if(state, exe));
	if (t == AST_WHILE || t == AST_UNTIL)
		return (execute_while(state, exe));
	if (t == AST_FOR)
		return (execute_for(state, exe));
	ft_assert(0);
	return (res_status(1));
}

t_execution_state	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_type	first_type;

	ft_assert(exe->node->children.len >= 1);
	first_type = ((t_ast_node *)exe->node->children.ctx)[0].node_type;
	if (first_type == AST_FUNCTION_DEF)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_func_def(state, exe));
	}
	if (first_type == AST_SIMPLE_COMMAND)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_simple_command(state, exe));
	}
	if (is_compound_ast(first_type))
	{
		ensure_redirs_initialized(exe);
		if (collect_redirects_from_ast(state, exe))
			return (res_status(AMBIGUOUS_REDIRECT));
		exe->node = vec_idx(&exe->node->children, 0);
		return (dispatch_compound(state, exe));
	}
	ft_assert(first_type == AST_SUBSHELL);
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

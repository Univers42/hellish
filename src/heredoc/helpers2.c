/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:31:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 15:14:27 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

static bool	node_is_groupable(t_ast_node *node)
{
	t_ast_node	*first;

	if (node->node_type == AST_SIMPLE_COMMAND)
		return (true);
	if (node->node_type == AST_COMMAND && node->children.len > 0
		&& node->children.ctx)
	{
		first = &((t_ast_node *)node->children.ctx)[0];
		if (first->node_type == AST_SIMPLE_COMMAND)
			return (true);
	}
	return (false);
}

static bool	should_skip_node(t_ast_node *node)
{
	if (!node)
		return (true);
	if (node->node_type == AST_PROC_SUB)
		return (true);
	if (node->node_type == AST_TOKEN)
		return (true);
	if (node->node_type == AST_WORD)
		return (true);
	return (false);
}

static void	recurse_non_redirect_child(t_shell *state,
									t_ast_node *node,
									size_t *idx)
{
	t_ast_node	*child;

	if (!node->children.ctx || *idx >= node->children.len)
	{
		(*idx)++;
		return ;
	}
	child = &((t_ast_node *)node->children.ctx)[*idx];
	if (!should_skip_node(child))
		gather_heredocs(state, child,
			(node->node_type == AST_COMMAND_PIPELINE));
	(*idx)++;
}

static void	handle_non_groupable_redirect(t_shell *state,
									t_ast_node *node,
									size_t *idx,
									bool in_pipeline)
{
	if (!node->children.ctx || *idx >= node->children.len)
	{
		(*idx)++;
		return ;
	}
	gather_heredoc(
		state,
		&((t_ast_node *)node->children.ctx)[*idx],
		(in_pipeline || node->node_type == AST_COMMAND_PIPELINE));
	(*idx)++;
}

static void	handle_grouped_redirects(t_shell *state,
								t_ast_node *node,
								size_t start)
{
	size_t	end;

	if (!node->children.ctx)
		return ;
	end = start;
	while (end < node->children.len
		&& ((t_ast_node *)node->children.ctx)[end].node_type
				== AST_REDIRECT)
		end++;
	process_redirect_group(state, node, start, end);
}

int	gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline)
{
	size_t		i;
	t_ast_node	*child;

	if (!node || should_skip_node(node))
		return (0);
	if (!node->children.ctx || node->children.len == 0)
	{
		if (node->node_type == AST_REDIRECT)
			gather_heredoc(state, node, in_pipeline);
		return (0);
	}
	i = 0;
	while (i < node->children.len && !get_g_sig()->should_unwind)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type != AST_REDIRECT)
		{
			recurse_non_redirect_child(state, node, &i);
			continue ;
		}
		if (!node_is_groupable(node))
		{
			handle_non_groupable_redirect(state, node, &i, in_pipeline);
			continue ;
		}
		handle_grouped_redirects(state, node, i);
		while (i < node->children.len
			&& ((t_ast_node *)node->children.ctx)[i].node_type == AST_REDIRECT)
			i++;
	}
	return (0);
}

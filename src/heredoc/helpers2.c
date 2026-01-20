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
	if (node->node_type == AST_COMMAND && node->children.len > 0)
	{
		first = &((t_ast_node *)node->children.ctx)[0];
		if (first->node_type == AST_SIMPLE_COMMAND)
			return (true);
	}
	return (false);
}

static void	recurse_non_redirect_child(t_shell *state,
									t_ast_node *node,
									size_t *idx)
{
	gather_heredocs(
		state,
		&((t_ast_node *)node->children.ctx)[*idx],
		(node->node_type == AST_COMMAND_PIPELINE));
	(*idx)++;
}

static void	handle_non_groupable_redirect(t_shell *state,
									t_ast_node *node,
									size_t *idx,
									bool in_pipeline)
{
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

	end = start;
	while (end < node->children.len
		&& ((t_ast_node *)node->children.ctx)[end].node_type
				== AST_REDIRECT)
		end++;
	process_redirect_group(state, node, start, end);
}

int	gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline)
{
	size_t	i;

	i = 0;
	while (i < node->children.len && !get_g_sig()->should_unwind)
	{
		if (((t_ast_node *)node->children.ctx)[i].node_type != AST_REDIRECT)
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
	if (node->node_type == AST_REDIRECT)
		handle_non_groupable_redirect(state, node, 0, in_pipeline);
	return (0);
}

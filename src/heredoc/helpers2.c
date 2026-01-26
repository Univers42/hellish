/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:31:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 02:50:10 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

bool	node_is_groupable(t_ast_node *node);
bool	should_skip_node(t_ast_node *node);
void	recurse_non_redirect_child(t_shell *state,
			t_ast_node *node,
			size_t *idx);

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

static void	skip_redirect_group(t_ast_node *node, size_t *i)
{
	while (*i < node->children.len
		&& ((t_ast_node *)node->children.ctx)[*i].node_type == AST_REDIRECT)
		(*i)++;
}

static void	process_child_node(t_shell *state, t_ast_node *node,
							size_t *i, bool in_pipeline)
{
	t_ast_node	*child;

	child = &((t_ast_node *)node->children.ctx)[*i];
	if (child->node_type != AST_REDIRECT)
	{
		recurse_non_redirect_child(state, node, i);
		return ;
	}
	if (!node_is_groupable(node))
	{
		handle_non_groupable_redirect(state, node, i, in_pipeline);
		return ;
	}
	handle_grouped_redirects(state, node, *i);
	skip_redirect_group(node, i);
}

int	gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline)
{
	size_t	i;

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
		process_child_node(state, node, &i, in_pipeline);
	return (0);
}

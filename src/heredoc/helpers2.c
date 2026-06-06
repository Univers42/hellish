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

/* Handle a single redirect child in a context where redirect grouping is
   not applicable (e.g. inside a compound command body rather than a
   pipeline).  We call gather_heredoc directly and advance the index. */
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

/* Collect the contiguous block of AST_REDIRECT siblings starting at
   `start` and pass the whole range to process_redirect_group in one
   call.  This ensures that multiple consecutive heredocs on the same
   command (`cmd <<A <<B`) are all registered before any are read. */
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

/* Advance *i past all contiguous AST_REDIRECT children.  Called after
   handle_grouped_redirects so the outer loop does not revisit them. */
static void	skip_redirect_group(t_ast_node *node, size_t *i)
{
	while (*i < node->children.len
		&& ((t_ast_node *)node->children.ctx)[*i].node_type == AST_REDIRECT)
		(*i)++;
}

/* Dispatch one child of `node` at index *i.  Non-redirect children are
   recursed into; redirect children are grouped when the node type supports
   it (node_is_groupable) or handled singly otherwise. */
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

/* Recursively pre-scan the whole AST for heredoc operators and write (or
   defer) their bodies before any execution starts.  This is the pre-scan
   pass called by execute_top_level.  gather_in_func is set to true for
   function/loop bodies so their heredocs are deferred (captured to node)
   rather than written eagerly: the body will be re-materialised at call
   time via materialize_heredoc, which is the only correct approach for
   code that runs multiple times. */
int	gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline)
{
	size_t	i;
	bool	saved_in_func;

	if (!node || should_skip_node(node))
		return (0);
	if (!node->children.ctx || node->children.len == 0)
	{
		if (node->node_type == AST_REDIRECT)
			gather_heredoc(state, node, in_pipeline);
		return (0);
	}
	saved_in_func = state->gather_in_func;
	if (node->node_type == AST_FUNCTION_DEF || node->node_type == AST_FOR
		|| node->node_type == AST_WHILE || node->node_type == AST_UNTIL)
		state->gather_in_func = true;
	i = 0;
	while (i < node->children.len && !get_g_sig()->should_unwind)
		process_child_node(state, node, &i, in_pipeline);
	state->gather_in_func = saved_in_func;
	return (0);
}

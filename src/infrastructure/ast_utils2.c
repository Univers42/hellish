/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 00:14:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"
#include "arith.h"

/* Post-order (children before parent) traversal: apply `f` to every node.
   Post-order is exactly what free_node needs -- free children first, then the
   parent -- but is also used by any pass that needs bottom-up processing. */
void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node))
{
	size_t	i;

	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal(vec_idx(&node->children, i), f);
		i++;
	}
	f(node);
}

/* Free all heap allocations belonging to a single node. The arith cache is
   freed via its own function (it may be shared). The start buffer is freed only
   when token.allocated is true (borrowed tokens share the lexer's buffer).
   full_word is an optional extra copy used by some expansion paths. */
void	free_node(t_ast_node *node)
{
	arith_cache_free(node->token.arith_cache);
	if (node->token.allocated)
		xfree(node->token.start);
	if (node->token.full_word)
	{
		if (node->token.full_word->allocated)
			xfree(node->token.full_word->start);
		xfree(node->token.full_word);
	}
	xfree(node->heredoc_body);
	xfree(node->children.ctx);
	*node = (t_ast_node){};
}

/* Recursively free an entire subtree by running free_node in post-order.
   The root pointer is also zeroed (via *node = {}), so a double-free is safe
   if free_ast is called again on the same variable. */
void	free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

/* Pastel fill colour for a node in the Graphviz DOT graph. The palette is
   chosen so each semantic category (pipeline, redirect, word, compound ...) has
   its own hue, making the graph visually scannable at a glance. */
char	*get_node_color(t_ast_type tn)
{
	if (tn == AST_COMMAND_PIPELINE)
		return ("#FFB6C1");
	if (tn == AST_REDIRECT)
		return ("#87CEEB");
	if (tn == AST_WORD || tn == AST_TOKEN)
		return ("#90EE90");
	if (tn == AST_SIMPLE_LIST || tn == AST_COMPOUND_LIST)
		return ("#DDA0DD");
	if (tn == AST_SIMPLE_COMMAND || tn == AST_COMMAND)
		return ("#F0E68C");
	if (tn == AST_SUBSHELL)
		return ("#FFB347");
	if (tn == AST_ASSIGNMENT_WORD)
		return ("#B0E0E6");
	if (tn == AST_PROC_SUB)
		return ("#98FB98");
	return ("#FFFFFF");
}

/* Graphviz shape for a node. Leaves (TOKEN/WORD) get ellipses; structural
   nodes (COMMAND, SIMPLE_COMMAND) get boxes; SUBSHELL gets an octagon to stand
   out; REDIRECT gets a diamond. All others default to box. */
char	*get_node_shape(t_ast_type tn)
{
	if (tn == AST_TOKEN || tn == AST_WORD)
		return ("ellipse");
	if (tn == AST_COMMAND || tn == AST_SIMPLE_COMMAND)
		return ("box");
	if (tn == AST_SUBSHELL)
		return ("octagon");
	if (tn == AST_REDIRECT)
		return ("diamond");
	return ("box");
}

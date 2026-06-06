/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:46 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Stamp every child node with a pointer to the full-word token covering the
   whole original raw text. Each child gets its own heap copy (xmalloc of the
   struct value) so free_ast can safely free them independently -- if we just
   stored the same pointer in all children we'd get a double-free at cleanup.
   The allocated=true flag on full_word tells the destructor to xfree it. */
static void	set_full_word_for_children(void *ctx, size_t len,
				t_token_old full_word)
{
	size_t		i;
	t_token_old	*p;

	i = 0;
	while (i < len)
	{
		p = xmalloc(sizeof(t_token_old));
		if (p)
			*p = full_word;
		((t_ast_node *)ctx)[i++].token.full_word = p;
	}
}

/* Recursively walk every child and call reparse_words on it. We skip
   AST_PROC_SUB because process substitution bodies have already been parsed
   as a sub-shell; re-parsing their word tokens would corrupt the AST since
   the tokens are borrowed from an inner parse context, not this one. */
static void	reparse_children_words(t_ast_node *node)
{
	size_t		i;
	t_ast_node	*child;

	i = 0;
	while (i < node->children.len)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type != AST_PROC_SUB)
			reparse_words(child);
		i++;
	}
}

/* The second-pass entry point for a whole AST subtree. For AST_WORD nodes
   with exactly one child (the raw token), replace the child vec with the
   fully parsed subtoken tree from reparse_word(). The temp/new_ctx dance
   avoids a double-free when reparse_word returns the same backing allocation
   (it may reuse the child vec if it only adds one node). The full_word pointer
   is stamped on every new child so the expander can reconstruct the original
   text for error messages and ${!var} style indirect references. */
void	reparse_words(t_ast_node *node)
{
	t_ast_node	temp;
	t_token_old	full_word;
	t_token		tok;
	void		*new_ctx;
	size_t		new_len;

	if (node->node_type == AST_WORD)
	{
		ft_assert(node->children.len == 1);
		tok = ((t_ast_node *)node->children.ctx)[0].token;
		full_word = create_token_old(tok.start, tok.len, true);
		temp = *node;
		*node = reparse_word(tok);
		new_ctx = node->children.ctx;
		new_len = node->children.len;
		if (temp.children.ctx != new_ctx)
			free_ast(&temp);
		set_full_word_for_children(new_ctx, new_len, full_word);
	}
	reparse_children_words(node);
}

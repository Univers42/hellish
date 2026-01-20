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

// Helper: set full_word for all children of a node
static void	set_full_word_for_children(void *ctx, size_t len,
				t_token_old full_word)
{
	size_t	i;

	i = 0;
	while (i < len)
		((t_ast_node *)ctx)[i++].token.full_word = full_word;
}

// Helper: recursively reparse all children except AST_PROC_SUB
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

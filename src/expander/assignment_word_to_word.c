/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_word_to_word.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:26:49 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:26:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Turn the key token "NAME" into "NAME=". The old code did token.len++, which
   only works when the token points into the source buffer (the '=' is the next
   byte). In a cloned body the key is a strndup'd "NAME\0", so build an
   explicit "NAME=" instead. */
static void	append_eq_to_token(t_token *t)
{
	char	*ke;

	ke = xmalloc(t->len + 2);
	if (!ke)
		return ;
	ft_memcpy(ke, t->start, t->len);
	ke[t->len] = '=';
	ke[t->len + 1] = '\0';
	if (t->allocated)
		xfree(t->start);
	t->start = ke;
	t->len++;
	t->allocated = true;
}

/* Convert an AST_ASSIGNMENT_WORD into a plain AST_WORD so that generic word
   expansion can handle it.  The key token gets a "=" appended (making it
   "NAME=") and the right-hand-side children are flatly spliced in after it.
   Called before expand_word_glob_ctl for `export NAME=val` argv expansion,
   but NOT for pre-command assignments (those go through assignment_to_env). */
void	assignment_word_to_word(t_ast_node *node)
{
	t_ast_node	ret;
	t_ast_node	left;
	t_ast_node	right;
	size_t		i;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	ft_assert(node->node_type == AST_ASSIGNMENT_WORD);
	ft_assert(node->children.len == 2);
	left = ((t_ast_node *)node->children.ctx)[0];
	right = ((t_ast_node *)node->children.ctx)[1];
	append_eq_to_token(&left.token);
	vec_push(&ret.children, &left);
	if (right.node_type == AST_WORD)
	{
		i = -1;
		while (++i < right.children.len)
			vec_push(&ret.children, vec_idx(&right.children, i));
		xfree(right.children.ctx);
	}
	else
		vec_push(&ret.children, &right);
	xfree(node->children.ctx);
	*node = ret;
}

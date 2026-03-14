/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils5.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

static t_token	clone_token(t_token tok)
{
	t_token	out;

	out = tok;
	if (tok.allocated && tok.start)
		out.start = ft_strndup(tok.start, tok.len);
	if (tok.full_word.present && tok.full_word.start)
		out.full_word.start = ft_strndup(tok.full_word.start,
				tok.full_word.len);
	return (out);
}

t_ast_node	clone_ast(t_ast_node *src)
{
	t_ast_node	dst;
	t_ast_node	child_copy;
	size_t		i;

	dst.node_type = src->node_type;
	dst.token = clone_token(src->token);
	dst.has_redirect = src->has_redirect;
	dst.redir_idx = src->redir_idx;
	vec_init(&dst.children);
	dst.children.elem_size = sizeof(t_ast_node);
	i = 0;
	while (i < src->children.len)
	{
		child_copy = clone_ast(vec_idx(&src->children, i));
		vec_push(&dst.children, &child_copy);
		i++;
	}
	return (dst);
}

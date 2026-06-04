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

/* Each token owns its own full_word allocation, so a clone must duplicate the
   struct (not share the pointer) to avoid a double free. The shallow clone
   borrows the start (allocated=false); the deep clone owns a strdup'd copy. */
static t_token_old	*dup_full_word(t_token_old *src, bool deep)
{
	t_token_old	*out;

	if (!src)
		return (NULL);
	out = malloc(sizeof(t_token_old));
	if (!out)
		return (NULL);
	*out = *src;
	out->allocated = false;
	if (deep && src->present && src->start)
	{
		out->start = ft_strndup(src->start, src->len);
		out->allocated = true;
	}
	return (out);
}

static t_token	clone_token(t_token tok)
{
	t_token	out;

	out = tok;
	out.allocated = false;
	out.full_word = dup_full_word(tok.full_word, false);
	return (out);
}

static t_token	deep_clone_token(t_token tok)
{
	t_token	out;

	out = tok;
	if (tok.start)
	{
		out.start = ft_strndup(tok.start, tok.len);
		out.allocated = true;
	}
	out.full_word = dup_full_word(tok.full_word, true);
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
	dst.negate = src->negate;
	if (src->heredoc_body)
		dst.heredoc_body = ft_strdup(src->heredoc_body);
	else
		dst.heredoc_body = NULL;
	vec_init(&dst.children);
	dst.children.elem_size = sizeof(t_ast_node);
	if (src->children.len)
		vec_ensure_space_n(&dst.children, src->children.len);
	i = 0;
	while (i < src->children.len)
	{
		child_copy = clone_ast(vec_idx(&src->children, i));
		vec_push(&dst.children, &child_copy);
		i++;
	}
	return (dst);
}

t_ast_node	deep_clone_ast(t_ast_node *src)
{
	t_ast_node	dst;
	t_ast_node	child_copy;
	size_t		i;

	dst.node_type = src->node_type;
	dst.token = deep_clone_token(src->token);
	dst.has_redirect = src->has_redirect;
	dst.redir_idx = src->redir_idx;
	dst.negate = src->negate;
	if (src->heredoc_body)
		dst.heredoc_body = ft_strdup(src->heredoc_body);
	else
		dst.heredoc_body = NULL;
	vec_init(&dst.children);
	dst.children.elem_size = sizeof(t_ast_node);
	if (src->children.len)
		vec_ensure_space_n(&dst.children, src->children.len);
	i = 0;
	while (i < src->children.len)
	{
		child_copy = deep_clone_ast(vec_idx(&src->children, i));
		vec_push(&dst.children, &child_copy);
		i++;
	}
	return (dst);
}

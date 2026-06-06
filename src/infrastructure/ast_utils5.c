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
	out = xmalloc(sizeof(t_token_old));
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

/* Shallow-clone a token: copy all fields but clear allocated (we borrow the
   original's start pointer without owning it) and drop the arith_cache (the
   receiver recomputes it if needed). full_word gets a new struct but still
   borrows the start buffer (allocated=false); safe for short-lived use. */
static t_token	clone_token(t_token tok)
{
	t_token	out;

	out = tok;
	out.allocated = false;
	out.arith_cache = NULL;
	out.full_word = dup_full_word(tok.full_word, false);
	return (out);
}

/* Deep-clone a token: strdup the start buffer and the full_word buffer so the
   copy is completely independent of the original lexer allocation. Used by
   deep_clone_ast for command-substitution fork paths where the parent's tree
   may be freed before the forked child finishes using it. */
static t_token	deep_clone_token(t_token tok)
{
	t_token	out;

	out = tok;
	out.arith_cache = NULL;
	if (tok.start)
	{
		out.start = ft_strndup(tok.start, tok.len);
		out.allocated = true;
	}
	out.full_word = dup_full_word(tok.full_word, true);
	return (out);
}

/* Shallow-clone an entire AST subtree: new node structs and child vectors are
   allocated, but token start buffers are borrowed (not owned). Safe as long as
   the original tree outlives all clones. Used internally where lifetime is
   guaranteed; otherwise use deep_clone_ast. */
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

/* Deep-clone: every string buffer is strdup'd; the copy is fully independent.
   Used when forking for command substitution -- the child needs its own tree
   because the parent frees it on the next REPL iteration while the child
   is still running. */
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

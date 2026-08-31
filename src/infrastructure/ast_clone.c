/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_clone.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast.h"
#include "libft.h"

/* Everything a clone copies EXCEPT the token and the children -- the flat
   fields plus a fresh child vector sized for what is coming.
     It lives in one function because clone_ast and deep_clone_ast copy the
   node field by field, and a field added to only one of them is invisible
   until something depends on it.  That is not hypothetical: `glued` was
   added to both node struct and clone_ast, the function-body path runs
   through deep_clone_ast, and a process substitution inside a function
   silently lost its value again.  Now there is one list to update. */
void	ast_clone_scalars(t_ast_node *dst, t_ast_node *src)
{
	dst->node_type = src->node_type;
	dst->has_redirect = src->has_redirect;
	dst->redir_idx = src->redir_idx;
	dst->negate = src->negate;
	dst->glued = src->glued;
	dst->case_term = src->case_term;
	dst->heredoc_body = NULL;
	if (src->heredoc_body)
		dst->heredoc_body = ft_strdup(src->heredoc_body);
	vec_init(&dst->children);
	dst->children.elem_size = sizeof(t_ast_node);
	if (src->children.len)
		vec_ensure_space_n(&dst->children, src->children.len);
}

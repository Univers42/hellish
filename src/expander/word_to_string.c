/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   word_to_string.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:09:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:09:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static bool	is_envvar_token(t_tt tt)
{
	return (tt == TT_ENVVAR || tt == TT_DQENVVAR);
}

static void	append_token_text(t_string *s, t_token curr, size_t total_children)
{
	if (curr.start)
		vec_push_nstr(s, curr.start, (size_t)curr.len);
	else if (curr.len == 0 && is_envvar_token(curr.tt))
	{
		if (total_children == 1)
			vec_push_char(s, '$');
	}
}

/* Build a t_string from a word AST node (concatenate token contents).
   Silently skip unexpected/non-token children. */
t_string	word_to_string(t_ast_node node)
{
	t_string	s;
	size_t		i;
	t_ast_node	*child;

	vec_init(&s);
	s.elem_size = 1;
	if (!node.children.ctx)
		return (s);
	i = -1;
	while (++i < node.children.len)
	{
		child = &((t_ast_node *)node.children.ctx)[i];
		if (child->node_type != AST_TOKEN)
			break ;
		append_token_text(&s, child->token, node.children.len);
	}
	return (s);
}

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

t_string	word_to_string(t_ast_node node)
{
	t_token		curr;
	t_string	s;
	size_t		i;

	vec_init(&s);
	s.elem_size = 1;
	i = 0;
	while (i < node.children.len)
	{
		if (!node.children.ctx)
			break;
		if (((t_ast_node *)node.children.ctx)[i].node_type != AST_TOKEN)
			break;
		curr = ((t_ast_node *)node.children.ctx)[i].token;
		if (curr.tt == TT_WORD || curr.tt == TT_SQWORD
			|| curr.tt == TT_DQWORD || curr.tt == TT_DQENVVAR
			|| curr.tt == TT_ENVVAR)
		{
			/* If start is present, push its contents (may be empty string for $''/$"").
			   If start is NULL and token is an envvar with len==0, emit literal '$'. */
			if (curr.start)
			{
				/* Push exactly curr.len bytes; zero-length means push nothing. */
				vec_push_nstr(&s, curr.start, (size_t)curr.len);
			}
			else if (curr.len == 0 && (curr.tt == TT_DQENVVAR || curr.tt == TT_ENVVAR))
			{
				/* If the word consists only of this envvar token (e.g. echo "$"),
				   treat it as a literal '$'. If there's additional children (e.g.
				   $'') then it's an explicit empty expansion and should be skipped. */
				if (node.children.len == 1)
				{
					char ch = '$';
					vec_push(&s, &ch);
				}
				/* else: explicit empty expansion (do nothing) */
			}
		}
		else
		{
			// skip unexpected tokens, do not crash
		}
		i++;
	}
	return (s);
}

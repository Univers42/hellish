/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_env_vars.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:37:05 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:37:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	expand_env_vars(t_shell *state, t_ast_node *node)
{
	t_token	*curr_tt;
	size_t	i;

	if (!node->children.ctx)
		return ;
	i = 0;
	while (i < node->children.len)
	{
		/* defensive: if children buffer is corrupted or contains unexpected
		   node types, abort expansion to avoid crashes */
		if (((t_ast_node *)node->children.ctx)[i].node_type != AST_TOKEN)
			return ;
		curr_tt = &((t_ast_node *)node->children.ctx)[i].token;
		if (curr_tt->tt == TT_WORD || curr_tt->tt == TT_SQWORD
			|| curr_tt->tt == TT_DQWORD)
		{
			/* nothing to do for plain words */
		}
		else if (curr_tt->tt == TT_DQENVVAR || curr_tt->tt == TT_ENVVAR)
		{
			/* Special-case zero-length var names: if followed immediately by
			   a quoted empty token (e.g. $'' or $"") that should expand to
			   empty string. Otherwise treat bare $ as literal '$'. */
			if (curr_tt->len == 0)
			{
				if (i + 1 < node->children.len)
				{
					t_ast_node next = ((t_ast_node *)node->children.ctx)[i + 1];
					if (next.node_type == AST_TOKEN && (next.token.tt == TT_SQWORD || next.token.tt == TT_DQWORD) && next.token.len == 0)
					{
						/* empty quoted RHS after $ -> empty expansion */
						curr_tt->start = "";
						curr_tt->len = 0;
						curr_tt->allocated = false;
					}
					else
					{
						/* bare $ -> leave literal dollar */
						curr_tt->start = "$";
						curr_tt->len = 1;
						curr_tt->allocated = false;
					}
				}
				else
				{
					/* no following token: bare $ */
					curr_tt->start = "$";
					curr_tt->len = 1;
					curr_tt->allocated = false;
				}
			}
			else
			{
				expand_token(state, curr_tt);
			}
		}
		else
			ft_assert("Unreachable" == 0);
		i++;
	}
}

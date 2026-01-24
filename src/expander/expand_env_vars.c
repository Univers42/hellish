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

static bool	next_is_quoted_empty(t_ast_node *node, size_t i)
{
	t_ast_node	nxt;

	if (i + 1 >= node->children.len)
		return (false);
	nxt = ((t_ast_node *)node->children.ctx)[i + 1];
	return ((nxt.node_type == AST_TOKEN
			&& (nxt.token.tt == TT_SQWORD || nxt.token.tt == TT_DQWORD)
			&& nxt.token.len == 0));
}

static void	process_env_token(t_shell *state, t_ast_node *node,
				t_token *curr_tt, size_t i)
{
	if (curr_tt->len == 0)
	{
		if (next_is_quoted_empty(node, i))
		{
			curr_tt->start = "";
			curr_tt->len = 0;
			curr_tt->allocated = false;
		}
		else
		{
			curr_tt->start = "$";
			curr_tt->len = 1;
			curr_tt->allocated = false;
		}
	}
	else
		expand_token(state, curr_tt);
}

void	expand_env_vars(t_shell *state, t_ast_node *node)
{
	t_token	*curr_tt;
	size_t	i;

	if (!node->children.ctx)
		return ;
	i = 0;
	while (i < node->children.len)
	{
		if (((t_ast_node *)node->children.ctx)[i].node_type != AST_TOKEN)
			return ;
		curr_tt = &((t_ast_node *)node->children.ctx)[i].token;
		if (curr_tt->tt == TT_WORD || curr_tt->tt == TT_SQWORD
			|| curr_tt->tt == TT_DQWORD)
			;
		else if (curr_tt->tt == TT_DQENVVAR || curr_tt->tt == TT_ENVVAR)
			process_env_token(state, node, curr_tt, i);
		else
			ft_assert(0);
		i++;
	}
}

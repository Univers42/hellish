/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

// Helper: reparse single-quoted region
void	reparse_squote(t_ast_node *ret, int *i, t_token t)
{
	t_reparser	rp;

	create_reparser(&rp, *ret, t, i);
	ft_assert(rp.current_token.start[rp.i++] == '\'');
	rp.prev_start = rp.i;
	while (rp.i < rp.current_token.len
		&& rp.current_token.start[rp.i] != '\'')
		rp.i++;
	push_subtoken_node(&rp.current_node, rp.current_token,
		create_interval(rp.prev_start, rp.i), TT_SQWORD);
	ft_assert(rp.current_token.start[rp.i++] == '\'');
	*i = rp.i;
	*ret = rp.current_node;
}

// Helper: reparse backslash escape
void	reparse_bs(t_ast_node *ret, int *i, t_token t)
{
	t_reparser	rp;

	create_reparser(&rp, *ret, t, i);
	ft_assert(rp.current_token.start[rp.i++] == '\\');
	rp.prev_start = rp.i;
	if (rp.i == rp.current_token.len)
		rp.prev_start--;
	else
		rp.i++;
	push_subtoken_node(&rp.current_node, rp.current_token,
		create_interval(rp.prev_start, rp.i), TT_SQWORD);
	*i = rp.i;
	*ret = rp.current_node;
}

// Helper: reparse normal word region
void	reparse_norm_word(t_ast_node *ret, int *i, t_token t)
{
	t_reparser	rp;

	create_reparser(&rp, *ret, t, i);
	rp.prev_start = rp.i;
	while (rp.i < rp.current_token.len
		&& !is_special_char(rp.current_token.start[rp.i])
		&& rp.current_token.start[rp.i] != '\\')
		rp.i++;
	push_subtoken_node(&rp.current_node, rp.current_token,
		create_interval(rp.prev_start, rp.i), TT_WORD);
	*i = rp.i;
	*ret = rp.current_node;
}

// Helper: main loop for reparsing a word token
void	loop_node_rp(t_reparser *rp)
{
	while (rp->i < rp->current_token.len)
	{
		if (rp->current_token.start[rp->i] == '"')
			reparse_dquote(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '\\')
			reparse_bs(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '\'')
			reparse_squote(&rp->current_node, &rp->i, rp->current_token);
		else if (rp->current_token.start[rp->i] == '$')
			reparse_envvar(&rp->current_node, &rp->i,
				rp->current_token, TT_ENVVAR);
		else if (is_space(rp->current_token.start[rp->i]))
			ft_assert(0);
		else
			reparse_norm_word(&rp->current_node, &rp->i, rp->current_token);
	}
}

t_ast_node	reparse_word(t_token t)
{
	t_ast_node	ret;
	t_reparser	rp;

	ret = create_node_type(AST_WORD);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	create_reparser(&rp, ret, t, &(int){0});
	loop_node_rp(&rp);
	ret = rp.current_node;
	return (ret);
}

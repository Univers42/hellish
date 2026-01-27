/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	create_subtoken_node(t_token t, int offset,
								int end_offset, t_tt tt)
{
	t_ast_node	ret;

	ret = create_node_tok(AST_TOKEN,
			create_token(t.start + offset, end_offset - offset, tt));
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	return (ret);
}

bool	is_simple_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON || tt == TT_OR || tt == TT_AND || tt == TT_AMPERSAND)
		return (true);
	return (false);
}

bool	is_semicolon_or_newline_before_brace_right(t_ast_node *ret,
											t_deque_tok *tokens)
{
	size_t	len;
	t_tt	next_tt;
	t_tt	last_tt;

	len = ret->children.len;
	if (len == 0)
		return (false);
	last_tt = ((t_ast_node *)ret->children.ctx)[len - 1].token.tt;
	next_tt = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	return ((last_tt == TT_SEMICOLON || last_tt == TT_NEWLINE)
		&& next_tt == TT_BRACE_RIGHT);
}

bool	is_end_token(t_deque_tok *tokens)
{
	t_tt	tt;

	tt = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	return (tt == TT_END);
}

bool	is_newline_token(t_deque_tok *tokens)
{
	t_tt	tt;

	tt = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	return (tt == TT_NEWLINE);
}

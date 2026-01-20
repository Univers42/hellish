/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_redirect.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:18 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 21:04:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

void	validate_next_token_is_properly_set_for_redirect(t_deque_tt *tokens,
													t_shell *state,
													t_parser *parser,
													t_ast_node ret)
{
	t_token	next;

	next = *(t_token *)deque_peek(&tokens->deqtok);
	if (!(next.tt == TT_WORD || next.tt == TT_SQWORD
			|| next.tt == TT_DQWORD
			|| next.tt == TT_ENVVAR
			|| next.tt == TT_DQENVVAR))
	{
		(void)unexpected(state, parser, ret, tokens);
		return ;
	}
}

t_ast_node	parse_redirect(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_ast_node	tmp_node;
	t_token		t;

	ret = (t_ast_node){.node_type = AST_REDIRECT};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	t = *(t_token *)deque_pop_start(&tokens->deqtok);
	if (!is_redirect(t.tt))
		return (unexpected(state, parser, ret, tokens));
	push_token_child(&ret, t);
	validate_next_token_is_properly_set_for_redirect(tokens,
		state, parser, ret);
	tmp_node = parse_word(tokens);
	vec_push(&ret.children, &tmp_node);
	return (ret);
}

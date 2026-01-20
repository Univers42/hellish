/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_redirect.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:18 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:10:26 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_redirect(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		t;

	ret = (t_ast_node){.node_type = AST_REDIRECT};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	t = *(t_token *)deque_pop_start(&tokens->deqtok);
	if (!is_redirect(t.tt))
		return (unexpected(state, parser, ret, tokens));
	{
		t_ast_node tmp_node = (t_ast_node){.node_type = AST_TOKEN, .token = t};
		vec_init(&tmp_node.children);
		tmp_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret.children, &tmp_node);
	}
	/* validate next token is a proper filename/token for redirection */
	{
		t_token next = *(t_token *)deque_peek(&tokens->deqtok);
		if (!(next.tt == TT_WORD || next.tt == TT_SQWORD
			|| next.tt == TT_DQWORD || next.tt == TT_ENVVAR
			|| next.tt == TT_DQENVVAR))
		{
			return (unexpected(state, parser, ret, tokens));
		}
	}
	{
		t_ast_node tmp_node = parse_word(tokens);
		vec_push(&ret.children, &tmp_node);
	}
	return (ret);
}

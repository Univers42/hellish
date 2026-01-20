/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:22:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_subshell(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_SUBSHELL};
	/* init children storage */
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	{ int val = TT_BRACE_LEFT; vec_push(&parser->parse_stack, &val); }
	peek_t = *(t_token *)deque_peek(&tokens->deqtok);
	if (peek_t.tt != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	(void)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node tmp_node = parse_compound_list(state, parser, tokens);
		/* ensure tmp_node children vector is initialized (parse_compound_list does this) */
		vec_push(&ret.children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (ret);
	peek_t = *(t_token *)deque_peek(&tokens->deqtok);
	if (peek_t.tt != TT_BRACE_RIGHT)
		return (unexpected(state, parser, ret, tokens));
	(void)deque_pop_start(&tokens->deqtok);
	vec_pop(&parser->parse_stack);
	return (ret);
}


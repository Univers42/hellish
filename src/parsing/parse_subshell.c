/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_subshell(t_shell *state, t_parser *parser, t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_SUBSHELL};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push_int(&parser->parse_stack, TT_BRACE_LEFT);
	peek_t = *(t_token *)deque_peek(&tokens->deqtok);
	if (peek_t.tt != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	peek_t = *(t_token *)deque_peek(&tokens->deqtok);
	if (peek_t.tt != TT_BRACE_RIGHT)
		return (unexpected(state, parser, ret, tokens));
	return (deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

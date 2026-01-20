/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_pipeline.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:22:12 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_pipeline(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;

	ret = (t_ast_node){.node_type = AST_COMMAND_PIPELINE};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	{
		t_ast_node tmp_node = parse_command(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (ret);
	{ int val = TT_PIPE; vec_push(&parser->parse_stack, &val); }
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_PIPE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
			(void)deque_pop_start(&tokens->deqtok);
		if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
			return (parser->res = RES_MoreInput, ret);
		{
			t_ast_node tmp_node = parse_command(state, parser, tokens);
			vec_push(&ret.children, &tmp_node);
		}
		if (parser->res != RES_OK)
			return (ret);
	}
	vec_pop(&parser->parse_stack);
	return (ret);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:11:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_command(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;

	/* Check for arithmetic expression (( which is not supported */
	if (next == TT_ARITH_START)
	{
		parser->res = RES_FatalError;
		state->last_cmd_status_res = res_status(1);
		return (ret);
	}

	if (next == TT_BRACE_LEFT)
	{
		{
			t_ast_node tmp_node = parse_subshell(state, parser, tokens);
			vec_push(&ret.children, &tmp_node);
		}
		if (parser->res != RES_OK)
			return (ret);
		while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
		{
			t_ast_node tmp_node = parse_redirect(state, parser, tokens);
			vec_push(&ret.children, &tmp_node);
			if (parser->res != RES_OK)
				return (ret);
		}
	}
	else
	{
		t_ast_node tmp_node = parse_simple_command(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
		if (parser->res != RES_OK)
			return (ret);
	}
	return (ret);
}

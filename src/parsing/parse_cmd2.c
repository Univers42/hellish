/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static bool	handle_func_def(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	func_node;

	func_node = parse_function_def(state, parser, tokens);
	vec_push(&ret->children, &func_node);
	if (parser->res != RES_OK)
		return (false);
	return (true);
}

static bool	dispatch_cmd(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_BRACE_LEFT)
		return (handle_subshell_case(state, parser, tokens, ret));
	if (is_compound_start(next))
		return (handle_compound_case(state, parser, tokens, ret));
	if (is_function_def(tokens))
		return (handle_func_def(state, parser, tokens, ret));
	return (handle_simple_command_case(state, parser, tokens, ret));
}

t_ast_node	parse_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_ARITH_START)
	{
		parser->res = RES_ERR;
		state->last_cmd_st_exe = res_status(1);
		return (ret);
	}
	if (!dispatch_cmd(state, parser, tokens, &ret))
		return (ret);
	return (ret);
}

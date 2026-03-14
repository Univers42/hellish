/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static bool	handle_subshell_case(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_subshell(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
	{
		tmp_node = parse_redirect(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
		if (parser->res != RES_OK)
			return (false);
	}
	return (true);
}

static bool	handle_compound_case(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;
	t_tt		next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_IF)
		tmp_node = parse_if_command(state, parser, tokens);
	else if (next == TT_WHILE)
		tmp_node = parse_while_command(state, parser, tokens);
	else if (next == TT_UNTIL)
		tmp_node = parse_until_command(state, parser, tokens);
	else
		tmp_node = parse_for_command(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
	{
		tmp_node = parse_redirect(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
		if (parser->res != RES_OK)
			return (false);
	}
	return (true);
}

static bool	handle_simple_command_case(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_simple_command(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	return (true);
}

t_ast_node	parse_command(t_shell *state, t_parser *parser, t_deque_tok *tokens)
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
	if (next == TT_BRACE_LEFT)
	{
		if (!handle_subshell_case(state, parser, tokens, &ret))
			return (ret);
	}
	else if (is_compound_start(next))
	{
		if (!handle_compound_case(state, parser, tokens, &ret))
			return (ret);
	}
	else if (is_function_def(tokens))
	{
		t_ast_node	func_node;

		func_node = parse_function_def(state, parser, tokens);
		vec_push(&ret.children, &func_node);
		if (parser->res != RES_OK)
			return (ret);
	}
	else
		if (!handle_simple_command_case(state, parser, tokens, &ret))
			return (ret);
	return (ret);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	handle_subshell_case(t_shell *state, t_parser *parser,
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

static t_ast_node	dispatch_compound(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_tt next)
{
	if (next == TT_IF)
		return (parse_if_command(state, parser, tokens));
	if (next == TT_WHILE)
		return (parse_while_command(state, parser, tokens));
	if (next == TT_UNTIL)
		return (parse_until_command(state, parser, tokens));
	if (next == TT_CASE)
		return (parse_case_command(state, parser, tokens));
	if (next == TT_LBRACE)
		return (parse_brace_group(state, parser, tokens));
	return (parse_for_command(state, parser, tokens));
}

bool	handle_compound_case(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;
	t_tt		next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	tmp_node = dispatch_compound(state, parser, tokens, next);
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

bool	handle_simple_command_case(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_simple_command(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	return (true);
}

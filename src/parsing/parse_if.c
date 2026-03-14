/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_if.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static bool	expect_token(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_tt expected)
{
	t_tt	next;

	(void)state;
	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
	{
		parser->res = RES_GETMOREINPUT;
		return (false);
	}
	if (next != expected)
	{
		parser->res = RES_ERR;
		return (false);
	}
	(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

static bool	parse_elif_chain(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	while (next == TT_ELIF)
	{
		(void)deque_pop_start(&tokens->deqtok);
		push_parsed_compound_list(state, parser, tokens, ret);
		if (parser->res != RES_OK)
			return (false);
		if (!expect_token(state, parser, tokens, TT_THEN))
			return (false);
		push_parsed_compound_list(state, parser, tokens, ret);
		if (parser->res != RES_OK)
			return (false);
		skip_newlines(tokens);
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	}
	if (next == TT_ELSE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		push_parsed_compound_list(state, parser, tokens, ret);
		if (parser->res != RES_OK)
			return (false);
	}
	return (true);
}

/*
** if compound_list then compound_list [elif ...] [else ...] fi
** AST_IF children: pairs of (condition, body), optional last = else body
*/
t_ast_node	parse_if_command(t_shell *state, t_parser *parser,
							t_deque_tok *tokens)
{
	t_ast_node	ret;

	init_ast_node_children(&ret, AST_IF);
	vec_push_int(&parser->parse_stack, TT_IF);
	(void)deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_token(state, parser, tokens, TT_THEN))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!parse_elif_chain(state, parser, tokens, &ret))
		return (ret);
	if (!expect_token(state, parser, tokens, TT_FI))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}

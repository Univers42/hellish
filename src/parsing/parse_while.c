/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_while.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Same semantics as expect_token in parse_if.c: skip leading newlines,
   demand exactly the expected keyword, signal RES_GETMOREINPUT on TT_END,
   or set RES_ERR on any other token. Separate copy here keeps the function
   count per-file manageable under the 42 norm. */
static bool	expect_kw(t_shell *state, t_parser *parser,
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

/* Parse: while compound_list do compound_list done
   AST_WHILE: children[0]=condition compound_list, children[1]=body.
   The `while` token is already consumed by the time we get here (the keyword
   classifier promoted it and the dispatcher popped it). */
t_ast_node	parse_while_command(t_shell *state, t_parser *parser,
								t_deque_tok *tokens)
{
	t_ast_node	ret;

	init_ast_node_children(&ret, AST_WHILE);
	vec_push_int(&parser->parse_stack, TT_WHILE);
	(void)deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_kw(state, parser, tokens, TT_DO))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_kw(state, parser, tokens, TT_DONE))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}

/* Parse: until compound_list do compound_list done
   Identical structure to while but the executor inverts the loop condition
   (loop while the compound-list returns non-zero). AST_UNTIL is a distinct
   node type so the executor does not need to inspect the keyword. */
t_ast_node	parse_until_command(t_shell *state, t_parser *parser,
								t_deque_tok *tokens)
{
	t_ast_node	ret;

	init_ast_node_children(&ret, AST_UNTIL);
	vec_push_int(&parser->parse_stack, TT_UNTIL);
	(void)deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_kw(state, parser, tokens, TT_DO))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_kw(state, parser, tokens, TT_DONE))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}

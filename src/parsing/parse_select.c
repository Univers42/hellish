/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_select.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Same "require this keyword or signal incomplete/error" logic as in the
   while/if parsers, for the `do` and `done` of a for/select loop. */
static bool	expect_loop_kw(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_tt expected)
{
	t_tt	next;

	(void)state;
	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
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

/* NAME [in wordlist [;|\n]] do compound_list done -- what follows the
   keyword of both `for` and `select`.  `ret` already carries the node
   type and the caller has pushed its keyword on parse_stack (the PS2
   depth view reads it); the keyword itself is already consumed. */
t_ast_node	parse_for_tail(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_ast_node ret)
{
	t_tt	next;

	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
		return (parser->res = RES_GETMOREINPUT, ret);
	if (!is_for_word(next))
		return (parser->res = RES_ERR, ret);
	ret.token = pop_tok(tokens);
	if (for_head(state, parser, tokens, &ret) != 1)
		return (ret);
	if (!expect_loop_kw(state, parser, tokens, TT_DO))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!expect_loop_kw(state, parser, tokens, TT_DONE))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}

/* select NAME [in wordlist]; do compound_list; done -- bash's menu loop,
   issue #122.  Grammatically a `for` with another keyword: same head,
   same in-clause, same body, so the AST has the same shape (AST_SELECT,
   token = NAME, children = words then body) and only execute_select
   reads it differently. */
t_ast_node	parse_select_command(t_shell *state, t_parser *parser,
							t_deque_tok *tokens)
{
	t_ast_node	ret;

	init_ast_node_children(&ret, AST_SELECT);
	(void)deque_pop_start(&tokens->deqtok);
	vec_push_int(&parser->parse_stack, TT_SELECT);
	return (parse_for_tail(state, parser, tokens, ret));
}

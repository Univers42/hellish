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

/* Consume a required keyword token. Newlines before it are skipped because
   POSIX allows `if ...\nthen` on separate lines. TT_END means the user hit
   Enter before closing the construct -- signal RES_GETMOREINPUT so the REPL
   can prompt for continuation. Any other mismatch is a syntax error. */
static bool	expect_token(t_shell *state, t_parser *parser,
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

/* Parse a single `elif condition then body` branch. The `elif` token has
   already been confirmed by parse_elif_chain; we pop it here and parse the
   condition compound-list, then require `then`, then the body. Children are
   appended to ret (the if-node) as alternating condition/body pairs. */
static bool	parse_elif_one(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	(void)deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, ret);
	if (parser->res != RES_OK)
		return (false);
	if (!expect_token(state, parser, tokens, TT_THEN))
		return (false);
	push_parsed_compound_list(state, parser, tokens, ret);
	return (parser->res == RES_OK);
}

/* Consume zero or more `elif ... then ...` branches followed by an optional
   `else ...` branch. The executor finds elif/else bodies by walking the
   children array: odd positions are conditions, even are bodies, and the
   final child with no following condition is the else body (if present). */
static bool	parse_elif_chain(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	while (next == TT_ELIF)
	{
		if (!parse_elif_one(state, parser, tokens, ret))
			return (false);
		skip_newlines(tokens);
		next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
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

/* Parse: if compound_list then compound_list [elif ...] [else ...] fi
   AST_IF children: alternating condition/body compound_list nodes, with an
   optional unpaired trailing child for the else body. The TT_IF token is
   popped here (it was already recognised by the keyword classifier). */
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

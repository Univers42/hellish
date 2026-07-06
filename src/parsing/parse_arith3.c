/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arith3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* The arithmetic command: (( expr )).  AST_ARITH_CMD carries the raw
   expression text in its token; the executor evaluates it and maps a
   non-zero value to exit status 0 (bash semantics). */
t_ast_node	parse_arith_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;

	(void)state;
	init_ast_node_children(&ret, AST_ARITH_CMD);
	ret.token = collect_arith_span(parser, tokens);
	return (ret);
}

/* Expect one keyword for the for-arith tail, tolerating newlines and
   requesting more input at EOF (same contract as the classic for parser). */
static bool	fa_expect(t_parser *parser, t_deque_tok *tokens, t_tt expected)
{
	t_tt	next;

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

/* Parse: for (( init ; cond ; step )) [;] do compound_list done
   AST_FOR_ARITH: children = three AST_WORD expression slices followed by
   the body compound-list (position-based, like the classic for). */
t_ast_node	parse_for_arith(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	init_ast_node_children(&ret, AST_FOR_ARITH);
	vec_push_int(&parser->parse_stack, TT_FOR);
	ret.token = collect_arith_span(parser, tokens);
	if (parser->res != RES_OK)
		return (ret);
	if (!push_arith_slices(&ret, ret.token))
		return (parser->res = RES_ERR, ret);
	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_SEMICOLON)
		(void)deque_pop_start(&tokens->deqtok);
	if (!fa_expect(parser, tokens, TT_DO))
		return (ret);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	if (!fa_expect(parser, tokens, TT_DONE))
		return (ret);
	vec_pop(&parser->parse_stack);
	return (ret);
}

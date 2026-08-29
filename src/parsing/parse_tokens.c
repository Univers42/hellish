/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_tokens.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:21:46 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Top-level parse entry point called from the REPL after tokenisation.
   Runs parse_simple_list to build the full AST, then -- if that succeeded --
   pops and discards the TT_END sentinel, and runs the two re-parse passes
   that upgrade raw TT_WORD tokens: reparse_words (splits composite words
   like `a"b"c` into parts) and reparse_assignment_words (promotes `VAR=val`
   to assignment nodes). The AST is optionally printed in dot format for
   debugging when PRINT_AST is set. */
t_ast_node	parse_tokens(t_shell *state, t_parser *parser, t_deque_tok *tokens)
{
	t_tt		tt;
	t_ast_node	ret;
	t_token		tmp;

	parser->res = RES_OK;
	ret = parse_simple_list(state, parser, tokens);
	if (parser->res == RES_OK)
	{
		tmp = pop_tok(tokens);
		tt = tmp.tt;
		(void)tt;
		reparse_all(state, &ret);
	}
	if (PRINT_AST)
		print_ast_dot(state, ret);
	return (ret);
}

/* Streaming variant: parse ONE newline-delimited top-level range and leave
   the rest of the token stream in the deque for the next call. *more is
   true when a range boundary stopped the parse (execute this range, come
   back); false when the whole stream is consumed — only then is the TT_END
   sentinel popped, exactly as parse_tokens does. Both reparse passes run
   per range, so the executed subtree is indistinguishable from a whole-
   batch parse. This is what caps the parse footprint at one range instead
   of one file: the caller resets the arena between ranges. */
t_ast_node	parse_tokens_range(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, bool *more)
{
	t_ast_node	ret;

	parser->res = RES_OK;
	parser->stream_more = false;
	ret = parse_simple_list(state, parser, tokens);
	*more = parser->stream_more;
	if (parser->res == RES_OK)
	{
		if (!*more)
			(void)deque_pop_start(&tokens->deqtok);
		reparse_all(state, &ret);
	}
	if (PRINT_AST)
		print_ast_dot(state, ret);
	return (ret);
}

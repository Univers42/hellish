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
		tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
		tt = tmp.tt;
		(void)tt;
		reparse_words(&ret);
		reparse_assignment_words(&ret);
	}
	if (PRINT_AST)
		print_ast_dot(state, ret);
	return (ret);
}

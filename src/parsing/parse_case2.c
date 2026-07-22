/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_case2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse the `WORD in` header of a case command: consume the subject word,
   skip newlines, then require the literal word "in". Both TT_END positions
   trigger RES_GETMOREINPUT so the REPL can prompt for more. Missing `in`
   after the word is a hard syntax error. */
static bool	parse_case_word_in(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	(void)state;
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, false);
	push_parsed_word(tokens, ret);
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, false);
	if (!is_kw_in(pk(tokens), tokens->base))
		return (parser->res = RES_ERR, false);
	(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

/* Parse: case WORD in [pattern) list ;;]... esac
   AST_CASE: children[0]=subject word, children[1..]=AST_CASE_ITEM nodes.
   The loop terminates on `esac` or TT_END (which triggers more-input). Each
   case item is parsed by parse_case_item, which consumes its own `;;`. The
   final deque_pop discards the `esac` token; vec_pop removes the TT_CASE
   entry from the parse stack that was pushed at the start. */
t_ast_node	parse_case_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_ast_node	item;

	init_ast_node_children(&ret, AST_CASE);
	vec_push_int(&parser->parse_stack, TT_CASE);
	(void)deque_pop_start(&tokens->deqtok);
	if (!parse_case_word_in(state, parser, tokens, &ret))
		return (ret);
	skip_newlines(tokens);
	while (pk(tokens)->tt != TT_ESAC)
	{
		if (pk(tokens)->tt == TT_END)
			return (parser->res = RES_GETMOREINPUT, ret);
		item = parse_case_item(state, parser, tokens);
		ast_push_child(&ret, &item);
		if (parser->res != RES_OK)
			return (ret);
		skip_newlines(tokens);
	}
	return ((void)deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

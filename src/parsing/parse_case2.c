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
	if (!is_kw_in(pk(tokens)))
		return (parser->res = RES_ERR, false);
	(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

/* case WORD in [pattern) list ;;]... esac */
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
		vec_push(&ret.children, &item);
		if (parser->res != RES_OK)
			return (ret);
		skip_newlines(tokens);
	}
	return ((void)deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_function.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 17:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/*
** Check if current tokens match function definition pattern:
** WORD ( ) { ... }
** Returns true if first 3 tokens are WORD BRACE_LEFT BRACE_RIGHT
*/
bool	is_function_def(t_deque_tok *tokens)
{
	t_token	*t0;
	t_token	*t1;
	t_token	*t2;

	if (tokens->deqtok.len < 3)
		return (false);
	t0 = (t_token *)deque_idx(&tokens->deqtok, 0);
	t1 = (t_token *)deque_idx(&tokens->deqtok, 1);
	t2 = (t_token *)deque_idx(&tokens->deqtok, 2);
	return (t0->tt == TT_WORD
		&& t1->tt == TT_BRACE_LEFT
		&& t2->tt == TT_BRACE_RIGHT);
}

static t_ast_node	parse_func_body(t_shell *state, t_parser *parser,
					t_deque_tok *tokens)
{
	t_ast_node	body;
	t_tt		next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_LBRACE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		body = parse_compound_list(state, parser, tokens);
		if (parser->res != RES_OK)
			return (body);
		skip_newlines(tokens);
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
		if (next == TT_RBRACE)
			(void)deque_pop_start(&tokens->deqtok);
		else if (next == TT_END)
			parser->res = RES_GETMOREINPUT;
		else
			return (unexpected(state, parser, body, tokens));
	}
	else
		body = parse_compound_list(state, parser, tokens);
	return (body);
}

/*
** Parse a function definition: name() { compound_list }
** AST_FUNCTION_DEF: token = name, children[0] = body
*/
t_ast_node	parse_function_def(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		name_tok;
	t_ast_node	body;

	name_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	(void)deque_pop_start(&tokens->deqtok);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
	{
		ret = create_node_tok(AST_FUNCTION_DEF, name_tok);
		parser->res = RES_GETMOREINPUT;
		return (ret);
	}
	body = parse_func_body(state, parser, tokens);
	ret = create_node_tok(AST_FUNCTION_DEF, name_tok);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret.children, &body);
	return (ret);
}

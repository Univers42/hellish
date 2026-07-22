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

/* Look ahead three tokens to decide if we are at a function definition.
   We require WORD ( ) as the first three tokens -- the `()` is TT_BRACE_LEFT
   + TT_BRACE_RIGHT. No whitespace or other tokens between them; the lexer
   would have split them otherwise. The function body (`{ ... }` or a
   compound-list) is NOT checked here -- parse_func_body handles that. */
bool	is_function_def(t_deque_tok *tokens)
{
	t_ltoken	*t0;
	t_ltoken	*t1;
	t_ltoken	*t2;

	if (tokens->deqtok.len < 3)
		return (false);
	t0 = (t_ltoken *)deque_idx(&tokens->deqtok, 0);
	t1 = (t_ltoken *)deque_idx(&tokens->deqtok, 1);
	t2 = (t_ltoken *)deque_idx(&tokens->deqtok, 2);
	if (t0->tt == TT_WORD && t0->len > 0 && t0->start[t0->len - 1] == '=')
		return (false);
	return (t0->tt == TT_WORD
		&& t1->tt == TT_BRACE_LEFT
		&& t2->tt == TT_BRACE_RIGHT);
}

/* Parse the function body after `name()`. If it starts with TT_LBRACE
   we enter the explicit `{ compound_list }` path and consume the braces.
   Otherwise (e.g. a compound command like `while ... done`) we hand off
   directly to parse_compound_list. The TT_RBRACE-vs-TT_END check prevents
   an infinite-input prompt when the user types `f() {` and forgets `}`. */
static t_ast_node	parse_func_body(t_shell *state, t_parser *parser,
					t_deque_tok *tokens)
{
	t_ast_node	body;
	t_tt		next;

	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_LBRACE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		body = parse_compound_list(state, parser, tokens);
		if (parser->res != RES_OK)
			return (body);
		skip_newlines(tokens);
		next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
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

/* Parse a function definition: name() { compound_list }
   Pops the three guaranteed tokens (name, `(`, `)`) before doing anything
   else, then skips newlines. If input ends right after `)` we return early
   with RES_GETMOREINPUT so the REPL prompts for the body. AST_FUNCTION_DEF:
   ret.token = the name token, children[0] = body AST. */
t_ast_node	parse_function_def(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		name_tok;
	t_ast_node	body;

	name_tok = ltok2tok(*(t_ltoken *)deque_pop_start(&tokens->deqtok));
	(void)deque_pop_start(&tokens->deqtok);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_END)
	{
		ret = create_node_tok(AST_FUNCTION_DEF, name_tok);
		parser->res = RES_GETMOREINPUT;
		return (ret);
	}
	body = parse_func_body(state, parser, tokens);
	ret = create_node_tok(AST_FUNCTION_DEF, name_tok);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	ast_push_child(&ret, &body);
	return (ret);
}

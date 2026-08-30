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

/* How many tokens the `function` form's head occupies, or 0 if this is not
   that form.

     function name { ... }        2 tokens before the body
     function name () { ... }     4

   `function` is matched as a plain WORD, not promoted to a reserved word, so
   a command genuinely named "function" still runs and nothing in the POSIX
   grammar shifts. Only the shape "function <name>" at the head of a command
   is claimed -- which is what bash and zsh both do (#71 item 5.1), and the
   single construct that takes the zsh plugin corpus from 4 loading to 5. */
static size_t	func_kw_len(t_deque_tok *tokens, t_ltoken *t0)
{
	t_ltoken	*t1;
	t_ltoken	*t2;
	t_ltoken	*t3;

	if (t0->tt != TT_WORD || t0->len != 8
		|| ft_strncmp(tokens->base + t0->off, "function", 8) != 0)
		return (0);
	t1 = (t_ltoken *)deque_idx(&tokens->deqtok, 1);
	if (t1->tt != TT_WORD || t1->len == 0)
		return (0);
	if (tokens->deqtok.len < 4)
		return (2);
	t2 = (t_ltoken *)deque_idx(&tokens->deqtok, 2);
	t3 = (t_ltoken *)deque_idx(&tokens->deqtok, 3);
	if (t2->tt == TT_BRACE_LEFT && t3->tt == TT_BRACE_RIGHT)
		return (4);
	return (2);
}

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

	if (tokens->deqtok.len < 2)
		return (false);
	t0 = (t_ltoken *)deque_idx(&tokens->deqtok, 0);
	if (t0->tt == TT_WORD && t0->len > 0
		&& (tokens->base + t0->off)[t0->len - 1] == '=')
		return (false);
	if (func_kw_len(tokens, t0) > 0)
		return (true);
	if (tokens->deqtok.len < 3)
		return (false);
	t1 = (t_ltoken *)deque_idx(&tokens->deqtok, 1);
	t2 = (t_ltoken *)deque_idx(&tokens->deqtok, 2);
	return (t0->tt == TT_WORD
		&& t1->tt == TT_BRACE_LEFT
		&& t2->tt == TT_BRACE_RIGHT);
}

/* Parse the function body after `name()`. Shared with the anonymous
   form (parse_func_anon.c): `() { ... }` has no name and no other
   difference, so it must not grow a second body parser.
     Parse the function body after `name()`. If it starts with TT_LBRACE
   we enter the explicit `{ compound_list }` path and consume the braces.
   Otherwise (e.g. a compound command like `while ... done`) we hand off
   directly to parse_compound_list. The TT_RBRACE-vs-TT_END check prevents
   an infinite-input prompt when the user types `f() {` and forgets `}`. */
t_ast_node	parse_func_body(t_shell *state, t_parser *parser,
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

/* Consume the definition head and return the NAME token. Three shapes, and
   the token count differs for each -- which is the whole reason this is not
   inline any more:

     name () { }        pop name, pop ( , pop )
     function name { }  pop `function`, pop name
     function name () { }  all four

   The name is always the token immediately after an optional `function`. */
static t_token	pop_func_head(t_shell *state, t_deque_tok *tokens)
{
	t_token	name_tok;
	size_t	head;

	head = func_kw_len(tokens, (t_ltoken *)deque_idx(&tokens->deqtok, 0));
	if (head > 0)
		(void)deque_pop_start(&tokens->deqtok);
	name_tok = pop_tok(tokens);
	if (head > 0 && zsh_mode(state))
		zfunc_names(tokens, &name_tok);
	if (head != 2)
	{
		(void)deque_pop_start(&tokens->deqtok);
		(void)deque_pop_start(&tokens->deqtok);
	}
	return (name_tok);
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

	name_tok = pop_func_head(state, tokens);
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

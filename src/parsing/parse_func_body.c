/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_func_body.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse the function body after `name()`. Shared with the anonymous
   form (parse_func_anon.c): `() { ... }` has no name and no other
   difference, so it must not grow a second body parser.
     If it starts with TT_LBRACE we enter the explicit `{ compound_list }`
   path and consume the braces. The TT_RBRACE-vs-TT_END check prevents an
   infinite-input prompt when the user types `f() {` and forgets `}`.
     Any other body is ONE compound command, which is bash's grammar:
   `f() ( ... )`, `f() while ...; done`, `f() if ...; fi`, `f() (( ... ))`.
   These went to parse_compound_list, which wants a list closed by a
   keyword and so reported end-of-file -- or, from -c, nothing at all --
   for every one of them. conda's shell hook is `__conda_exe() ( ... )`
   and it was the difference between a working conda and a dead rc. */
/* Remember the body's SOURCE SPAN on the body node's token, which no
   list or compound node uses otherwise.  ast_source_text() widens over
   every in-buffer token of the subtree, and the tokens the parser
   consumed -- `for`, `while`, `if`, `{`, `(`, `!` -- are not in it, so a
   body whose first statement began with one printed without it:
   `declare -f` on `f() { for x in a; do :; done; }` came back as
   `x in a; do :; done;`, which does not re-parse.  With the raw span
   pinned here the walk covers exactly what was written between the
   braces.  Nodes whose token is meaningful (a bodyless `f() for ...`
   keeps its name there) are left alone and fall back to the walk. */
static void	body_span(t_ast_node *body, const char *lo, const char *hi)
{
	if (body->token.len != 0 || body->token.start || hi <= lo)
		return ;
	body->token.start = (char *)lo;
	body->token.len = (int)(hi - lo);
	body->token.tt = TT_WORD;
}

/* The `{ compound_list }` body: the braces are consumed here, so this is
   the one place that knows where the text between them starts and ends. */
static t_ast_node	parse_brace_body(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	body;
	t_ltoken	*t;
	const char	*lo;

	t = (t_ltoken *)deque_peek(&tokens->deqtok);
	lo = tokens->base + t->off + t->len;
	(void)deque_pop_start(&tokens->deqtok);
	body = parse_compound_list(state, parser, tokens);
	if (parser->res != RES_OK)
		return (body);
	skip_newlines(tokens);
	t = (t_ltoken *)deque_peek(&tokens->deqtok);
	if (t->tt == TT_RBRACE)
	{
		body_span(&body, lo, tokens->base + t->off);
		(void)deque_pop_start(&tokens->deqtok);
	}
	else if (t->tt == TT_END)
		parser->res = RES_GETMOREINPUT;
	else
		return (unexpected(state, parser, body, tokens));
	return (body);
}

t_ast_node	parse_func_body(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	body;
	t_ltoken	*t;
	const char	*lo;

	t = (t_ltoken *)deque_peek(&tokens->deqtok);
	if (t->tt == TT_LBRACE)
		return (parse_brace_body(state, parser, tokens));
	lo = tokens->base + t->off;
	if (t->tt == TT_BRACE_LEFT || is_compound_start(t->tt))
		body = parse_body_pipeline(state, parser, tokens);
	else
		body = parse_compound_list(state, parser, tokens);
	t = (t_ltoken *)deque_peek(&tokens->deqtok);
	if (parser->res == RES_OK && t->tt != TT_END)
		body_span(&body, lo, tokens->base + t->off);
	return (body);
}

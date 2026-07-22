/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_array.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	is_valid_ident(char *s, int len);

/* arr=(a b c) / arr+=(a b c) compound array assignment. Recognised only
   in its exact shape: a word ending in '=' (optionally '+=') whose name
   part is a plain identifier, with the '(' ADJACENT in the source (token
   slices are contiguous input, so pointer arithmetic proves adjacency —
   `arr= (x)` is an assignment followed by a subshell and must NOT match,
   exactly as in bash). */

/* Does `w` look like NAME= or NAME+= with `open` immediately after it? */
static bool	is_array_assign_start(t_token *w, t_ltoken *open, char *base)
{
	int	n;

	if (w->tt != TT_WORD || !w->start || w->len < 2)
		return (false);
	if (open->tt != TT_BRACE_LEFT || w->start + w->len != base + open->off)
		return (false);
	if (w->start[w->len - 1] != '=')
		return (false);
	n = w->len - 1;
	if (n > 0 && w->start[n - 1] == '+')
		n--;
	if (n <= 0)
		return (false);
	return (is_valid_ident(w->start, n));
}

/* Try to consume WORD=( word... ) from the stream into an
   AST_ARRAY_ASSIGN node: child[0] is the raw key token (kept with its
   trailing '=' or '+=' so the expander can see the append flag), the
   rest are the element words, each a normal AST_WORD that the reparse
   pass refines like any other word. Newlines inside the parens are
   separators (bash allows multi-line arrays); end of input inside the
   parens asks for more. Returns false when the shape does not match and
   the caller should treat the word normally. */
bool	try_push_array_assign(t_parser *parser, t_deque_tok *tokens,
			t_ast_node *ret)
{
	t_ast_node	node;
	t_token		curr;

	curr = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (tokens->deqtok.len < 2 || !is_array_assign_start(&curr,
			(t_ltoken *)deque_idx(&tokens->deqtok, 1), tokens->base))
		return (false);
	node = create_node_type(AST_ARRAY_ASSIGN);
	vec_init(&node.children);
	node.children.elem_size = sizeof(t_ast_node);
	add_op_token_child(&node, pop_tok(tokens));
	(void)deque_pop_start(&tokens->deqtok);
	curr = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	while (curr.tt == TT_WORD || curr.tt == TT_NEWLINE)
	{
		if (curr.tt == TT_WORD)
			push_parsed_word(tokens, &node);
		else
			(void)deque_pop_start(&tokens->deqtok);
		curr = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	}
	if (curr.tt == TT_BRACE_RIGHT)
		(void)deque_pop_start(&tokens->deqtok);
	else if (curr.tt == TT_END)
		parser->res = RES_GETMOREINPUT;
	else
		parser->res = RES_ERR;
	return (ast_push_child(ret, &node), true);
}

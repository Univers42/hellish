/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arith2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* The arithmetic command `(( expr ))` and the C-style for header share one
   trick: the lexer split the expression into word/operator tokens, but all
   tokens are slices of the same contiguous input buffer, so the raw text
   is reconstructed by CHARACTER scan from the "((" opener and the token
   stream is then resynced past everything the span swallowed. */

/* Scan chars from the "((" at s[0]: paren depth starts at 2 and the span
   ends when it reaches 0.  Returns 0 and sets *end to the index of the
   first ')' of the final adjacent pair on success, 1 when the input ran
   out (caller should ask for more), 2 when the two closing parens are not
   adjacent (not an arithmetic command — a hard syntax error for us). */
static int	arith_span_chars(const char *s, int *end)
{
	int	depth;
	int	i;

	depth = 2;
	i = 2;
	while (s[i] && depth > 0)
	{
		if (s[i] == '(')
			depth++;
		else if (s[i] == ')')
			depth--;
		i++;
	}
	if (depth > 0)
		return (1);
	*end = i - 2;
	if (s[i - 2] != ')')
		return (2);
	return (0);
}

/* Drop every queued token that lives inside the character span we just
   consumed; the parser resumes at the first token past the closing "))". */
static void	arith_resync_tokens(t_deque_tok *tokens, const char *end)
{
	t_ltoken	*peek;

	peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	while (peek->tt != TT_END && peek->start < end)
	{
		(void)deque_pop_start(&tokens->deqtok);
		peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	}
}

/* Pop the TT_ARITH_START token and swallow its whole (( ... )) span.
   Returns the expression text as a synthetic word token.  Unterminated
   input requests more lines (interactive PS2 / EOF error in -c mode);
   a malformed close is a syntax error. */
t_token	collect_arith_span(t_parser *parser, t_deque_tok *tokens)
{
	t_token	open;
	t_token	span;
	int		end;
	int		r;

	open = ltok2tok(*(t_ltoken *)deque_pop_start(&tokens->deqtok));
	span = create_token(open.start, 0, TT_WORD);
	r = arith_span_chars(open.start, &end);
	if (r == 1)
		return (parser->res = RES_GETMOREINPUT, span);
	if (r == 2)
		return (parser->res = RES_ERR, span);
	span = create_token(open.start + 2, end - 2, TT_WORD);
	arith_resync_tokens(tokens, open.start + end + 2);
	return (span);
}

/* Split the for-arith header on its two top-level ';' and push the three
   expression slices as children (init, cond, step).  They are AST_TOKEN
   nodes, not AST_WORD: the word reparser must leave raw arithmetic text
   alone (it asserts on childless word nodes and would mangle operators).
   Nested parens shield their ';' — which cannot occur in arithmetic
   anyway, but the guard keeps a stray one from splitting mid-parenthesis.
   Returns false unless exactly three fields are present (bash: error). */
bool	push_arith_slices(t_ast_node *ret, t_token span)
{
	t_ast_node	w;
	int			i;
	int			depth;
	int			start;

	i = 0;
	depth = 0;
	start = 0;
	while (i <= span.len)
	{
		if (i < span.len && span.start[i] == '(')
			depth++;
		else if (i < span.len && span.start[i] == ')')
			depth--;
		else if (i == span.len || (span.start[i] == ';' && depth == 0))
		{
			w = create_node_type(AST_TOKEN);
			w.token = create_token(span.start + start, i - start, TT_WORD);
			ast_push_child(ret, &w);
			start = i + 1;
		}
		i++;
	}
	return (ret->children.len == 3);
}

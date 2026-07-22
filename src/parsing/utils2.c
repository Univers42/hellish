/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Slice a sub-range of an existing token and package it as a new AST_TOKEN
   node. Used by the re-parse pass (reparse_words) to split a composite
   TT_WORD like `"hello"world` into separate quoted and unquoted parts.
   The slice points into the original buffer -- no copy is made. */
t_ast_node	create_subtoken_node(t_token t, int offset,
								int end_offset, t_tt tt)
{
	t_ast_node	ret;

	ret = create_node_tok(AST_TOKEN,
			create_token(t.start + offset, end_offset - offset, tt));
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	return (ret);
}

/* True for the four operators that can appear between two pipelines in a
   simple-list: `;`, `||`, `&&`, and `&`. Newline is NOT included here (it
   terminates a simple-list, it does not continue it); that is why
   is_compund_list_op and is_simple_list_op are different functions. */
bool	is_simple_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON || tt == TT_OR || tt == TT_AND || tt == TT_AMPERSAND)
		return (true);
	return (false);
}

/* True when the last child of ret ended with `;` or newline AND the next
   token is `)` (TT_BRACE_RIGHT). This combination signals the end of a
   compound-list inside a subshell -- e.g. `(cmd;)`. Without this check the
   parser would try to parse `)` as a command and fail with a syntax error. */
bool	is_semicolon_or_newline_before_brace_right(t_ast_node *ret,
											t_deque_tok *tokens)
{
	size_t	len;
	t_tt	next_tt;
	t_tt	last_tt;

	len = ret->children.len;
	if (len == 0)
		return (false);
	last_tt = ((t_ast_node *)ret->children.ctx)[len - 1].token.tt;
	next_tt = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	return ((last_tt == TT_SEMICOLON || last_tt == TT_NEWLINE)
		&& next_tt == TT_BRACE_RIGHT);
}

/* Peek at the front of the deque and return true if it is TT_END. Called
   by the simple-list and compound-list loops to detect the end of token
   stream without consuming the sentinel. */
bool	is_end_token(t_deque_tok *tokens)
{
	t_tt	tt;

	tt = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	return (tt == TT_END);
}

/* Peek and return true if the next token is TT_NEWLINE. Used in the
   simple-list path to consume the optional trailing newline that terminates
   a statement without it being counted as end-of-input. */
bool	is_newline_token(t_deque_tok *tokens)
{
	t_tt	tt;

	tt = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	return (tt == TT_NEWLINE);
}

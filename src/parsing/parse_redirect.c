/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_redirect.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:18 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:28:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Placeholder validation hook -- currently a no-op with all parameters
   voided. Left in place as an extension point for stricter redirect-target
   validation (e.g. rejecting `> ` with no filename) without changing the
   calling code. */
void	validate_next_token_is_properly_set_for_redirect(t_deque_tok *tokens,
													t_shell *state,
													t_parser *parser,
													t_ast_node ret)
{
	t_token	next;

	(void)tokens;
	(void)state;
	(void)parser;
	(void)ret;
	next = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	(void)next;
}

/* Parse a redirect: op [procsub | word]. The operator token (>, >>, <, <<,
   etc.) is consumed first and becomes the first AST_TOKEN child. The target
   is either a process substitution `<(cmd)` / `>(cmd)` or a plain word (file
   name, heredoc label, fd number). Process subs take priority because the
   lexer already classified `<(` as TT_PROC_SUB_IN, so we just peek and
   branch. Missing or wrong target triggers unexpected() which sets RES_ERR
   and returns a partial node for the caller to propagate upward. */
t_ast_node	parse_redirect(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		t;
	t_token		next;

	ret = (t_ast_node){.node_type = AST_REDIRECT};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	t = pop_tok(tokens);
	if (!is_redirect(t.tt))
		return (unexpected(state, parser, ret, tokens));
	push_token_child(&ret, t);
	next = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (next.tt == TT_PROC_SUB_IN || next.tt == TT_PROC_SUB_OUT)
		ast_push_child(&ret, (t_ast_node[])
		{parse_proc_sub(state, parser, tokens)});
	else if (next.tt == TT_WORD || next.tt == TT_SQWORD
		|| next.tt == TT_DQWORD || next.tt == TT_ENVVAR
		|| next.tt == TT_DQENVVAR)
		ast_push_child(&ret, (t_ast_node[]){parse_word(tokens)});
	else
		return (unexpected(state, parser, ret, tokens));
	return (ret);
}

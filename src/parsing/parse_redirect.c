/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_redirect.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:18 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

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
	next = *(t_token *)deque_peek(&tokens->deqtok);
	(void)next;
}

t_ast_node	parse_redirect(t_shell *state, t_parser *parser, t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		t;
	t_token		next;

	ret = (t_ast_node){.node_type = AST_REDIRECT};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	t = *(t_token *)deque_pop_start(&tokens->deqtok);
	if (!is_redirect(t.tt))
		return (unexpected(state, parser, ret, tokens));
	push_token_child(&ret, t);
	next = *(t_token *)deque_peek(&tokens->deqtok);
	if (next.tt == TT_PROC_SUB_IN || next.tt == TT_PROC_SUB_OUT)
		vec_push(&ret.children, (t_ast_node[])
		{parse_proc_sub(state, parser, tokens)});
	else if (next.tt == TT_WORD || next.tt == TT_SQWORD
		|| next.tt == TT_DQWORD || next.tt == TT_ENVVAR
		|| next.tt == TT_DQENVVAR)
		vec_push(&ret.children, (t_ast_node[]){parse_word(tokens)});
	else
		return (unexpected(state, parser, ret, tokens));
	return (ret);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:18:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse a subshell command: ( compound_list ). The leading `(` is TT_BRACE_LEFT
   (the lexer's operator token), not TT_LBRACE (the keyword). We push
   TT_BRACE_LEFT onto the parse stack so error messages can tell the user
   which open `(` is unclosed. The trailing `)` must be present; if not,
   unexpected() sets RES_ERR. */
t_ast_node	parse_subshell(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_SUBSHELL};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push_int(&parser->parse_stack, TT_BRACE_LEFT);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok));
	if (peek_t.tt != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok));
	if (peek_t.tt != TT_BRACE_RIGHT)
		return (unexpected(state, parser, ret, tokens));
	return (deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

/* Parse a brace group: { compound_list ; } -- runs in the current shell, no
   fork. The braces are keyword tokens (TT_LBRACE / TT_RBRACE), distinct from
   the operator tokens that open/close subshells (TT_BRACE_LEFT/RIGHT). The
   trailing `}` must be preceded by `;` or newline (the grammar enforces this
   via is_separator_before_terminator in compound_list.c). */
t_ast_node	parse_brace_group(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_BRACE_GROUP};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push_int(&parser->parse_stack, TT_LBRACE);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok));
	if (peek_t.tt != TT_LBRACE)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok));
	if (peek_t.tt != TT_RBRACE)
		return (unexpected(state, parser, ret, tokens));
	return (deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

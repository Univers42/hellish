/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_coproc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	is_valid_ident(char *s, int len);

/* coproc [NAME] command: run `command` asynchronously behind a two-way
   pipe. NAME (default COPROC) receives [0]=read fd, [1]=write fd, and
   NAME_PID the child pid. The optional NAME is present only before a
   COMPOUND command (bash grammar); a bare `coproc cmd args` is the simple
   form with the default name. The reclassify pass keeps the token after
   `coproc NAME` in command position, so a `{`/keyword there is a real
   compound opener we can detect two tokens ahead. */

/* Is the operand at index i a NAME introducing a named compound coproc?
   That needs a valid-identifier WORD followed by a compound opener. */
static bool	coproc_has_name(t_deque_tok *tokens)
{
	t_token	*w;
	t_token	*nx;

	if (tokens->deqtok.len < 2)
		return (false);
	w = (t_token *)deque_idx(&tokens->deqtok, 0);
	nx = (t_token *)deque_idx(&tokens->deqtok, 1);
	if (w->tt != TT_WORD || !is_valid_ident(w->start, w->len))
		return (false);
	return (is_compound_start(nx->tt) || nx->tt == TT_BRACE_LEFT);
}

/* Build the AST_COPROC node: node.token is the NAME (empty => default
   COPROC), child[0] is the wrapped command parsed by parse_command. */
bool	handle_coproc_case(t_shell *state, t_parser *parser,
			t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	node;
	t_ast_node	inner;

	node = create_node_type(AST_COPROC);
	vec_init(&node.children);
	node.children.elem_size = sizeof(t_ast_node);
	(void)deque_pop_start(&tokens->deqtok);
	if (coproc_has_name(tokens))
		node.token = *(t_token *)deque_pop_start(&tokens->deqtok);
	inner = parse_command(state, parser, tokens);
	ast_push_child(&node, &inner);
	ast_push_child(ret, &node);
	return (parser->res == RES_OK);
}

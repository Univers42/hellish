/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils4.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 20:51:13 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Thin wrapper around unexpected() that discards the return value. Callers
   that do not need the partial AST back use this to avoid a -Wunused-value
   warning while keeping the code readable (no explicit cast to void). */
void	handle_unexpected_token(t_shell *state,
								t_parser *parser,
								t_ast_node ret,
								t_deque_tok *tokens)
{
	(void)unexpected(state, parser, ret, tokens);
}

/* Initialise an AST node of the given type with an empty children vector.
   Every parser function calls this or the create_node_type equivalent at its
   start; doing it in one place ensures elem_size is always set correctly
   before the first vec_push. */
void	init_ast_node_children(t_ast_node *node, t_ast_type type)
{
	*node = create_node_type(type);
	vec_init(&node->children);
	node->children.elem_size = sizeof(t_ast_node);
}

/* Wrap a raw token in an AST_TOKEN node and push it as a child of parent.
   Used when we need to store an operator or keyword token in the AST (e.g.
   the `;` between pipelines in a simple-list, the `|` in a pipeline). */
void	push_token_child(t_ast_node *parent, t_token tok)
{
	t_ast_node	tmp_node;

	tmp_node = create_node_tok(AST_TOKEN, tok);
	vec_init(&tmp_node.children);
	tmp_node.children.elem_size = sizeof(t_ast_node);
	ast_push_child(parent, &tmp_node);
}

/* Parse one command and push it as a child of ret. Used by parse_pipeline to
   push the first command (before any `|`); subsequent pipe stages are pushed
   directly in process_pipeline_pipes. */
void	push_cmd_parsed(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens,
						t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_command(state, parser, tokens);
	ast_push_child(ret, &tmp_node);
}

/* Parse one pipeline and push it as a child of parent. Used by both
   parse_simple_list and parse_compound_list; shared here rather than
   inlined to keep the call sites under the norm's per-function line limit. */
void	push_parsed_pipeline_child(t_shell *state,
							t_parser *parser,
							t_deque_tok *tokens,
							t_ast_node *parent)
{
	t_ast_node	tmp_node;

	tmp_node = parse_pipeline(state, parser, tokens);
	ast_push_child(parent, &tmp_node);
}

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

void	handle_unexpected_token(t_shell *state,
								t_parser *parser,
								t_ast_node ret,
								t_deque_tok *tokens)
{
	(void)unexpected(state, parser, ret, tokens);
}

void	init_ast_node_children(t_ast_node *node, t_ast_type type)
{
	*node = create_node_type(type);
	vec_init(&node->children);
	node->children.elem_size = sizeof(t_ast_node);
}

void	push_token_child(t_ast_node *parent, t_token tok)
{
	t_ast_node	tmp_node;

	tmp_node = create_node_tok(AST_TOKEN, tok);
	vec_init(&tmp_node.children);
	tmp_node.children.elem_size = sizeof(t_ast_node);
	vec_push(&parent->children, &tmp_node);
}

void	push_cmd_parsed(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens,
						t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_command(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
}

void	push_parsed_pipeline_child(t_shell *state,
							t_parser *parser,
							t_deque_tok *tokens,
							t_ast_node *parent)
{
	t_ast_node	tmp_node;

	tmp_node = parse_pipeline(state, parser, tokens);
	vec_push(&parent->children, &tmp_node);
}

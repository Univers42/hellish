/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils3.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 20:50:55 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

void	push_parsed_word(t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_word(tokens);
	vec_push(&ret->children, &tmp_node);
}

void	push_parsed_redirect(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_redirect(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
}

void	push_parsed_proc_sub(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_proc_sub(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
}

void	push_parsed_compound_list(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_compound_list(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
}

bool	check_res_ok(t_parser *res)
{
	return (res->res == RES_OK);
}

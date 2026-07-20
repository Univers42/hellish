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

/* Convenience wrappers that call the parse_* function and push the result
   onto ret->children in one step. These reduce the boilerplate throughout the
   parser: every compound command needs to push sub-lists, redirects, and
   words, and the two-line parse+push pattern repeats dozens of times. */
void	push_parsed_word(t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_word(tokens);
	ast_push_child(ret, &tmp_node);
}

void	push_parsed_redirect(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_redirect(state, parser, tokens);
	ast_push_child(ret, &tmp_node);
}

void	push_parsed_proc_sub(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_proc_sub(state, parser, tokens);
	ast_push_child(ret, &tmp_node);
}

void	push_parsed_compound_list(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_compound_list(state, parser, tokens);
	ast_push_child(ret, &tmp_node);
}

/* One-liner helper used in boolean expressions after a push_parsed_* call
   to keep the caller's expression short without breaking the 42 norm. */
bool	check_res_ok(t_parser *res)
{
	return (res->res == RES_OK);
}

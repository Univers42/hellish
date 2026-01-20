/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_simple_cmd.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:11:02 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
	t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_SIMPLE_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (!is_simple_cmd_token(next))
		return (unexpected(state, res, ret, tokens));
	while (1)
	{
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
		if (next == TT_WORD)
		{
			t_ast_node tmp_node = parse_word(tokens);
			vec_push(&ret.children, &tmp_node);
		}
		else if (is_redirect(next))
		{
			t_ast_node tmp_node = parse_redirect(state, res, tokens);
			vec_push(&ret.children, &tmp_node);
			if (res->res != RES_OK)
				return (ret);
		}
		else if (is_proc_sub(next))
		{
			t_ast_node tmp_node = parse_proc_sub(state, res, tokens);
			vec_push(&ret.children, &tmp_node);
			if (res->res != RES_OK)
				return (ret);
		}
		else
			break ;
	}
	return (ret);
}
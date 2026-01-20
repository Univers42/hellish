/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_simple_cmd.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 20:47:33 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static bool	parse_and_push_simple_cmd_child(t_shell *state, t_parser *res,
											t_deque_tt *tokens, t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_WORD)
		return (push_parsed_word(tokens, ret), true);
	else if (is_redirect(next))
		return (push_parsed_redirect(state, res, tokens, ret),
			check_res_ok(res));
	else if (is_proc_sub(next))
		return (push_parsed_proc_sub(state, res, tokens, ret),
			check_res_ok(res));
	return (false);
}

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
	while (ST_SCANNING)
		if (!parse_and_push_simple_cmd_child(state, res, tokens, &ret))
			break ;
	return (ret);
}

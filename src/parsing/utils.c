/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:09:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 00:14:49 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	is_redirect(t_tt tt)
{
	if (tt == TT_REDIRECT_LEFT
		|| tt == TT_REDIRECT_RIGHT
		|| tt == TT_APPEND
		|| tt == TT_HEREDOC)
		return (true);
	return (false);
}

bool	is_proc_sub(t_tt tt)
{
	return (tt == TT_PROC_SUB_IN || tt == TT_PROC_SUB_OUT);
}

bool	is_redirect_target(t_tt tt)
{
	return (tt == TT_WORD || tt == TT_SQWORD || tt == TT_DQWORD
		|| tt == TT_ENVVAR || tt == TT_DQENVVAR || is_proc_sub(tt));
}

bool	is_simple_cmd_token(t_tt tt)
{
	return (tt == TT_WORD || is_redirect(tt) || is_proc_sub(tt));
}

bool	is_compund_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON
		|| tt == TT_OR
		|| tt == TT_AND
		|| tt == TT_NEWLINE
		|| tt == TT_AMPERSAND)
		return (true);
	return (false);
}

t_ast_node	parse_word(t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		tmp;
	t_ast_node	token_node;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	token_node = create_node_tok(AST_TOKEN, tmp);
	vec_init(&token_node.children);
	token_node.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret.children, &token_node);
	return (ret);
}

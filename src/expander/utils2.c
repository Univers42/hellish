/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 02:28:49 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:20:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parser.h"
#include "shell.h"
#include "../parsing/parser_private.h"

int	get_default_src_fd(t_tt tt)
{
	if (tt == TT_REDIRECT_LEFT || tt == TT_HEREDOC || tt == TT_DUP_IN)
		return (STDIN_FILENO);
	return (STDOUT_FILENO);
}

bool	node_is_groupable(t_ast_node *node)
{
	t_ast_node	*first;

	if (node->node_type == AST_SIMPLE_COMMAND)
		return (true);
	if (node->node_type == AST_COMMAND && node->children.len > 0
		&& node->children.ctx)
	{
		first = &((t_ast_node *)node->children.ctx)[0];
		if (first->node_type == AST_SIMPLE_COMMAND)
			return (true);
	}
	return (false);
}

bool	is_redirect(t_tt tt)
{
	if (tt == TT_REDIRECT_LEFT
		|| tt == TT_REDIRECT_RIGHT
		|| tt == TT_APPEND
		|| tt == TT_HEREDOC)
		return (true);
	return (false);
}

void	proc_sub_update_depth(t_token curr, int *depth)
{
	if (curr.tt == TT_BRACE_LEFT || curr.tt == TT_PROC_SUB_IN
		|| curr.tt == TT_PROC_SUB_OUT)
		(*depth)++;
	else if (curr.tt == TT_BRACE_RIGHT)
		(*depth)--;
}

int	handle_end_token(t_parser *parser, t_deque_tok *tokens)
{
	parser->res = RES_GETMOREINPUT;
	tokens->looking_for = ')';
	return (1);
}

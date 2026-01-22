/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Reap any zombie background children (non-blocking) */
void	reap_background_children(void)
{
	int	status;

	while (waitpid(-1, &status, WNOHANG) > 0)
		;
}

bool	should_execute(t_exe_res prev_status, t_tt prev_op)
{
	if (prev_status.c_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR || prev_op == TT_AMPERSAND);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AMPERSAND)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

/* Find the next & or ; operator in the children list starting from index i.
   Returns the index of the operator, or children.len if not found.
   Also sets *found_amp to true if & was found before ; */
size_t	find_next_separator(t_ast_node *node, size_t start, bool *found_amp)
{
	size_t		i;
	t_ast_node	*child;

	*found_amp = false;
	i = start;
	while (i < node->children.len)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type == AST_TOKEN)
		{
			if (child->token.tt == TT_AMPERSAND)
			{
				*found_amp = true;
				return (i);
			}
			if (child->token.tt == TT_SEMICOLON
				|| child->token.tt == TT_NEWLINE)
				return (i);
		}
		i++;
	}
	return (node->children.len);
}

t_exe_res	execute_simple_list(t_shell *state, t_executable_node *exe)
{
	t_exe_res	status;
	size_t		i;
	size_t		sep_idx;
	bool		is_background;

	reap_background_children();
	status = res_status(0);
	i = 0;
	while (i < exe->node->children.len)
	{
		sep_idx = find_next_separator(exe->node, i, &is_background);
		if (is_background)
			status = execute_range_background(state, exe, i, sep_idx);
		else
			status = execute_range(state, exe, i, sep_idx);
		i = sep_idx + 1;
	}
	reap_background_children();
	return (status);
}

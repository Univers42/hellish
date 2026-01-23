/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_simple_child.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 00:21:30 by marvin            #+#    #+#             */
/*   Updated: 2026/01/23 00:21:30 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* handle a single child node of a simple command */
int	process_simple_child(t_shell *state, t_expander_simple_cmd *exp,
				t_executable_cmd *ret, t_vec_int *redirects)
{
	char	*path;

	if (exp->curr->node_type == AST_WORD)
		return (expand_simple_cmd_word(state, exp, ret));
	else if (exp->curr->node_type == AST_ASSIGNMENT_WORD)
		return (expand_simple_cmd_assignment(state, exp, ret));
	else if (exp->curr->node_type == AST_REDIRECT)
		return (expand_simple_cmd_redir(state, exp, redirects));
	else if (exp->curr->node_type == AST_PROC_SUB)
	{
		path = expand_proc_sub(state, exp->curr);
		if (path)
		{
			vec_push(&ret->argv, &path);
			exp->found_first = true;
		}
		return (0);
	}
	else if (exp->curr->node_type == AST_TOKEN)
		return (0);
	return (1);
}

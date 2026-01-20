/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser4.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

void	reparse_dq_bs(t_ast_node *ret, int *i, t_token t)
{
	ft_assert(t.start[*i] == '\\');
	(*i)++;
	if (ft_strchr("\"$\\", t.start[*i]))
		push_subtoken_node(ret, t, create_interval(*i - 1, *i + 1), TT_SQWORD);
	else
		push_subtoken_node(ret, t, create_interval(*i - 1, *i), TT_SQWORD);
	(*i)++;
}

bool	reparse_special_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	int		prev_start;
	char	c;

	prev_start = *i;
	c = t.start[*i];
	if (c != '?' && c != '$')
		return (false);
	(*i)++;
	push_subtoken_node(ret, t, create_interval(prev_start, *i), tt);
	return (true);
}

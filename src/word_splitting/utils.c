/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:49:33 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 16:13:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

void	push_subtoken_node(t_ast_node *ret,
			t_token t,
			t_interval offset,
			t_tt tt)
{
	t_ast_node	tmp;

	tmp = create_subtoken_node(t, offset.start, offset.end, tt);
	tmp.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret->children, &tmp);
}

bool	is_valid_ident(char *s, int len)
{
	int			i;

	i = 0;
	if (len <= 0 || !is_var_name_p1(s[0]))
		return (false);
	while (i < len)
	{
		if (!is_var_name_p2(s[i++]))
			return (false);
	}
	return (true);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/libft.h"
#include "../shell.h"
#include <stdbool.h>

//[a-zA-Z_]
bool	is_var_name_p1(char c)
{
	if (ft_isalpha(c) || c == '_')
		return (true);
	return (false);
}

//[a-zA-Z0-9_]
bool	is_var_name_p2(char c)
{
	if (ft_isalnum(c) || c == '_')
		return (true);
	return (false);
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
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, tt);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	return (true);
}

void	reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	int	prev_start;

	ft_assert(t.start[(*i)++] == '$');
	prev_start = *i;
	if (reparse_special_envvar(ret, i, t, tt))
		return ;
	if (*i < t.len && is_var_name_p1(t.start[*i]))
		(*i)++;
	while (*i < t.len && is_var_name_p2(t.start[*i]))
	{
		(*i)++;
	}
	if (prev_start == *i + 1)
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start - 1, *i, TT_WORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	else
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, tt);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
}

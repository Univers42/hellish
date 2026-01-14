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

#include "shell.h"
#include <stdbool.h>
#include "../libft/libft.h"
#include <stddef.h>
# include "parser.h"
# include "decomposer.h"

void	reparse_dq_bs(t_ast_node *ret, int *i, t_token t)
{
	ft_assert(t.start[*i] == '\\');
	(*i)++;
	if (ft_strchr("\"$\\", t.start[*i]))
	{
		t_ast_node tmp = create_subtoken_node(t, *i, *i + 1, TT_SQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	else
	{
		t_ast_node tmp = create_subtoken_node(t, *i - 1, *i + 1, TT_SQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	(*i)++;
}

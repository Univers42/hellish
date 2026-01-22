/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   word_to_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:20:23 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:20:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

t_string	word_to_hrdoc_string(t_ast_node node)
{
	t_token		curr;
	t_string	s;
	size_t		i;

	vec_init(&s);
	i = 0;
	while (i < node.children.len)
	{
		ft_assert(((t_ast_node *)node.children.ctx)[i].node_type == AST_TOKEN);
		curr = ((t_ast_node *)node.children.ctx)[i].token;
		if (curr.tt == TT_WORD || curr.tt == TT_SQWORD
			|| curr.tt == TT_DQWORD)
			vec_push_nstr(&s, curr.start, curr.len);
		else if (curr.tt == TT_DQENVVAR || curr.tt == TT_ENVVAR)
		{
			char ch = '$'; vec_push(&s, &ch);
			vec_push_nstr(&s, curr.start, curr.len);
		}
		else
			ft_assert("Unreachable" == 0);
		i++;
	}
	return (s);
}

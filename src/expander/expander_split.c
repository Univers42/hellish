/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_split.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	split_envvar(t_shell *state, t_token *curr_t,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char	**things;
	int		i;

	if (!curr_t->start)
		return ;
	things = ft_split_str(curr_t->start, env_get_ifs(&state->env));
	if (things[0])
	{
		{
			t_ast_node tmp = new_env_node(things[0]);

			vec_push(&curr_node->children, &tmp);
		}
		i = 1;
		while (things[i])
		{
			vec_push(ret, curr_node);
			*curr_node = (t_ast_node){.node_type = AST_WORD};
			vec_init(&curr_node->children);
			curr_node->children.elem_size = sizeof(t_ast_node);
			{
				t_ast_node tmp = new_env_node(things[i]);

				vec_push(&curr_node->children, &tmp);
			}
			i++;
		}
	}
	else if (curr_node->children.len)
	{
		vec_push(ret, curr_node);
		*curr_node = (t_ast_node){.node_type = AST_WORD};
		vec_init(&curr_node->children);
		curr_node->children.elem_size = sizeof(t_ast_node);
	}
	free(things);
}


// node -> split node
t_vec_nd	split_words(t_shell *state, t_ast_node *node)
{
	t_vec_nd	ret;
	t_token		*curr_t;
	t_ast_node	curr_node;
	int			i;

	vec_init(&ret);
	ret.elem_size = sizeof(t_ast_node);
	curr_node = (t_ast_node){.node_type = AST_WORD};
	vec_init(&curr_node.children);
	curr_node.children.elem_size = sizeof(t_ast_node);
	i = -1;
	while (++i < (int)node->children.len)
	{
		ft_assert(((t_ast_node *)node->children.ctx)[i].node_type == AST_TOKEN);
		curr_t = &((t_ast_node *)node->children.ctx)[i].token;
		if (curr_t->tt == TT_WORD || curr_t->tt == TT_SQWORD
			|| curr_t->tt == TT_DQWORD || curr_t->tt == TT_DQENVVAR)
			vec_push(&curr_node.children, &((t_ast_node *)node->children.ctx)[i]);
		else if (curr_t->tt == TT_ENVVAR)
			split_envvar(state, curr_t, &curr_node, &ret);
		else
			ft_assert("Unreachable" == 0);
	}
	if (curr_node.children.len)
		vec_push(&ret, &curr_node);
	free(node->children.ctx);
	*node = (t_ast_node){};
	return (ret);
}

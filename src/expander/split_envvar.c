/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_envvar.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 11:28:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 12:53:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static void	push_and_reinit_curr_node(t_vec_nd *ret, t_ast_node *curr_node)
{
	if (curr_node->children.len)
		vec_push(ret, curr_node);
	*curr_node = (t_ast_node){.node_type = AST_WORD};
	vec_init(&curr_node->children);
	curr_node->children.elem_size = sizeof(t_ast_node);
}

/* helper: create an env node from new_start and
	push it into curr_node->children */
static void	push_new_env_child(t_ast_node *curr_node, char *new_start)
{
	t_ast_node	tmp;

	tmp = new_env_node(new_start);
	vec_push(&curr_node->children, &tmp);
}

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
		push_new_env_child(curr_node, things[0]);
		i = 1;
		while (things[i])
		{
			push_and_reinit_curr_node(ret, curr_node);
			push_new_env_child(curr_node, things[i]);
			i++;
		}
	}
	else if (curr_node->children.len)
		push_and_reinit_curr_node(ret, curr_node);
	free(things);
}

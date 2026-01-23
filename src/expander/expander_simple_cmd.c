/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_simple_cmd.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* initialize executable cmd return struct (vectors & elem_size) */
static void	init_executable_cmd(t_executable_cmd *ret)
{
	*ret = (t_executable_cmd){};
	vec_init(&ret->pre_assigns);
	ret->pre_assigns.elem_size = sizeof(t_env);
	vec_init(&ret->argv);
	ret->argv.elem_size = sizeof(char *);
}

/* If no argv produced but pre_assigns present, transfer them into shell env */
static void	apply_pre_assigns_if_assignment_only(t_shell *state,
				t_executable_cmd *ret)
{
	t_env	tmp;

	if (ret->argv.len == 0 && ret->pre_assigns.len > 0)
	{
		while (ret->pre_assigns.len)
		{
			tmp = *(t_env *)vec_pop(&ret->pre_assigns);
			env_set(&state->env, tmp);
		}
		free(ret->pre_assigns.ctx);
		vec_init(&ret->pre_assigns);
	}
}

/* node -> expanded executable cmd (concise, delegates to helpers) */
int	expand_simple_command(t_shell *state, t_ast_node *node,
		t_executable_cmd *ret, t_vec_int *redirects)
{
	t_expander_simple_cmd	exp;

	if (node == NULL || node->children.len == 0 || node->children.ctx == NULL)
		return (1);
	if (node->children.elem_size == 0)
		node->children.elem_size = sizeof(t_ast_node);
	exp = (typeof(exp)){};
	init_executable_cmd(ret);
	while (exp.i < node->children.len)
	{
		exp.curr = (t_ast_node *)vec_idx(&node->children, exp.i);
		exp.exit_stat = process_simple_child(state, &exp, ret, redirects);
		if (exp.exit_stat)
			return (exp.exit_stat);
		exp.i++;
	}
	apply_pre_assigns_if_assignment_only(state, ret);
	return (0);
}

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

int	expand_simple_command(t_shell *state, t_ast_node *node,
		t_executable_cmd *ret, t_vec_int *redirects)
{
	t_expander_simple_cmd	exp;

	if (node == NULL || node->children.len == 0 || node->children.ctx == NULL)
		return (1);
	/* defensive sanity: ensure child elem_size set so vec_idx arithmetic works */
	if (node->children.elem_size == 0)
		node->children.elem_size = sizeof(t_ast_node);
	exp = (typeof(exp)){};
	*ret = (t_executable_cmd){};
	/* init pre_assigns vector to hold t_env entries */
	vec_init(&ret->pre_assigns);
	ret->pre_assigns.elem_size = sizeof(t_env);
	vec_init(&ret->argv);
	ret->argv.elem_size = sizeof(char *);
	while (exp.i < node->children.len)
	{
		exp.curr = (t_ast_node *)vec_idx(&node->children, exp.i);
		if (exp.curr->node_type == AST_WORD)
			exp.exit_stat = expand_simple_cmd_word(state, &exp, ret);
		else if (exp.curr->node_type == AST_ASSIGNMENT_WORD)
			exp.exit_stat = expand_simple_cmd_assignment(state, &exp, ret);
		else if (exp.curr->node_type == AST_REDIRECT)
			exp.exit_stat = expand_simple_cmd_redir(state, &exp, redirects);
		else if (exp.curr->node_type == AST_PROC_SUB)
		{
			/* Expand process substitution and add result to argv */
			char *path = expand_proc_sub(state, exp.curr);
			if (path)
			{
				vec_push(&ret->argv, &path);
				exp.found_first = true;
			}
			exp.exit_stat = 0;
		}
		else if (exp.curr->node_type == AST_TOKEN)
		{
			/* Skip operator tokens that might appear in argv (like redirects) */
			exp.exit_stat = 0;
		}
		else
		{
#if TRACE_DEBUG
			ft_eprintf("debug: expand_simple_command unexpected node_type=%d at idx=%lu\n",
				(int)exp.curr->node_type, (unsigned long)exp.i);
#endif
			return (1);
		}
		if (exp.exit_stat)
			return (exp.exit_stat);
		exp.i++;
	}
	/* If expansion produced only assignments and no argv (assignment-only form),
	   apply those to the shell environment now to take ownership of their strings.
	   This prevents leaks and double-free later when the temporary cmd is freed. */
	if (ret->argv.len == 0 && ret->pre_assigns.len > 0)
	{
		/* Pop each pre_assign and insert it into the shell env (transfer ownership) */
		while (ret->pre_assigns.len)
		{
			t_env tmp = *(t_env *)vec_pop(&ret->pre_assigns);
			/* ensure exported flag is preserved */
			env_set(&state->env, tmp);
		}
		/* free the backing storage and reinit vector */
		free(ret->pre_assigns.ctx);
		vec_init(&ret->pre_assigns);
	}
	return (0);
}

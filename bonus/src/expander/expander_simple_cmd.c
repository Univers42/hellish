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

#include "libft.h"
#include "shell.h"
#include <stdbool.h>

bool	is_export(t_ast_node word)
{
	t_ast_node	c;

	if (word.children.len != 1)
		return (false);
	c = ((t_ast_node *)word.children.ctx)[0];
	if (c.token.tt != TT_WORD)
		return (false);
	if (ft_strncmp(c.token.start, "export", c.token.len))
		return (false);
	return (true);
}

int	expand_simple_cmd_assignment(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	if (!exp->found_first)
	{
		t_env tmp = assignment_to_env(state, exp->curr);
		vec_push(&ret->pre_assigns, &tmp);
	}
	else
	{
		assignment_word_to_word(exp->curr);
		if (exp->export)
			expand_word(state, exp->curr, &ret->argv, true);
		else
			expand_word(state, exp->curr, &ret->argv, false);
		if (g_should_unwind)
			return (1);
	}
	return (0);
}

int	expand_simple_cmd_redir(t_shell *state,
		t_expander_simple_cmd *exp, t_vec_int *redirects)
{
	int			redir_idx;

	if (redirect_from_ast_redir(state, exp->curr, &redir_idx))
	{
		g_should_unwind = 0; // ensure not treated as canceled
		return (AMBIGUOUS_REDIRECT);
	}
	{ int idx = redir_idx; vec_push(redirects, &idx); } // <-- changed
	return (0);
}

int	expand_simple_cmd_word(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	if (!exp->found_first && is_export(*exp->curr))
		exp->export = true;
	expand_word(state, exp->curr, &ret->argv, false);
	if (g_should_unwind)
		return (1);
	exp->found_first = true;
	return (0);
}

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
	ret->argv.elem_size = sizeof(char *); // <--- store pointers
	while (exp.i < node->children.len)
	{
		exp.curr = (t_ast_node *)vec_idx(&node->children, exp.i);
		if (exp.curr->node_type == AST_WORD)
			exp.exit_stat = expand_simple_cmd_word(state, &exp, ret);
		else if (exp.curr->node_type == AST_ASSIGNMENT_WORD)
			exp.exit_stat = expand_simple_cmd_assignment(state, &exp, ret);
		else if (exp.curr->node_type == AST_REDIRECT)
			exp.exit_stat = expand_simple_cmd_redir(state, &exp, redirects);
		else
		{
#if TRACE_DEBUG
			ft_eprintf("debug: expand_simple_command unexpected node_type=%d at idx=%lu\n",
				(int)exp.curr->node_type, (unsigned long)exp.i);
#endif
			return (1); /* unexpected node type — fail expansion */
		}
		if (exp.exit_stat)
			return (exp.exit_stat);
		exp.i++;
	}
	return (0);
}

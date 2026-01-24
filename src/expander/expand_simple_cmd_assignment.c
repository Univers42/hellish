/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_simple_cmd_assignment.c                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "lexer.h"
#include "sys.h"

/* Capture original full token when safe 
(assignment child's original word or word node) */
static t_token_old	capture_original_full_token(t_ast_node *node)
{
	t_token_old	full;
	t_ast_node	*orig;

	full = init_token_old();
	if (!node)
		return (full);
	if (node->node_type == AST_ASSIGNMENT_WORD && node->children.len > 1)
	{
		orig = &((t_ast_node *)node->children.ctx)[1];
		if (orig->node_type == AST_WORD)
			full = get_old_token(*orig);
	}
	else if (node->node_type == AST_WORD)
		full = get_old_token(*node);
	return (full);
}

/* Replace argv entries ending with '=' 
using provided full token (dup) */
static void	replace_argv_entries_with_full_token(t_vec *argv,
				t_token_old full)
{
	size_t	ai;
	char	*s;
	char	*orig;

	if (!argv || !argv->ctx || !full.present)
		return ;
	ai = 0;
	while (ai < argv->len)
	{
		s = ((char **)argv->ctx)[ai];
		if (s && s[0] && s[ft_strlen(s) - 1] == EQ)
		{
			orig = ft_strndup(full.start, full.len);
			free(((char **)argv->ctx)[ai]);
			((char **)argv->ctx)[ai] = orig;
		}
		ai++;
	}
}

/* Perform assignment -> word conversion, expand into argv and apply fixup.
   Returns 1 on unwind request, 0 otherwise. */
static int	expand_assignment_word_and_fixup(t_shell *state,
					t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	t_token_old	full;

	full = capture_original_full_token(exp->curr);
	assignment_word_to_word(exp->curr);
	if (exp->export)
		expand_word(state, exp->curr, &ret->argv, true);
	else
		expand_word(state, exp->curr, &ret->argv, false);
	if (get_g_sig()->should_unwind)
		return (1);
	replace_argv_entries_with_full_token(&ret->argv, full);
	return (0);
}

int	expand_simple_cmd_assignment(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	t_env		tmp;

	if (!exp->found_first)
	{
		tmp = assignment_to_env(state, exp->curr);
		vec_push(&ret->pre_assigns, &tmp);
		return (0);
	}
	if (expand_assignment_word_and_fixup(state, exp, ret))
		return (1);
	return (0);
}

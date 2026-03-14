/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_for.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static t_execution_state	run_body(t_shell *state, t_ast_node *body)
{
	t_executable_node	body_exe;
	t_ast_node			body_copy;
	t_execution_state	status;

	body_copy = clone_ast(body);
	body_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, &body_copy, true);
	status = execute_tree_node(state, &body_exe);
	free_ast(&body_copy);
	return (status);
}

static void	set_for_var(t_shell *state, char *name, char *val)
{
	t_env	var;

	var = env_create(ft_strdup(name), ft_strdup(val), false);
	env_set(&state->env, var);
}

static t_execution_state	for_word_loop(t_shell *state,
		t_ast_node *node, char *var_name, size_t wc)
{
	t_execution_state	status;
	size_t				i;
	char				*word_value;

	status = res_status(0);
	i = 0;
	while (i < wc)
	{
		word_value = expand_word_single(state,
				vec_idx(&node->children, i));
		set_for_var(state, var_name, word_value ? word_value : "");
		free(word_value);
		status = run_body(state, vec_idx(&node->children, wc));
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
		i++;
	}
	return (status);
}

/*
** for NAME [in wordlist ;] do compound_list done
** children = [word_list...] + compound_list(body)
*/
t_execution_state	execute_for(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	char				*var_name;
	size_t				word_count;

	ft_assert(exe->node->children.len >= 1);
	var_name = ft_strndup(exe->node->token.start, exe->node->token.len);
	word_count = exe->node->children.len - 1;
	status = for_word_loop(state, exe->node, var_name, word_count);
	free(var_name);
	return (status);
}

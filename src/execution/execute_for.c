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

int	handle_loop_ctl(t_shell *state);

/* Run the for-loop body IN PLACE (no per-iteration clone). Word expansion is
   non-destructive, so `body` survives for the next iteration; the AST owner
   frees it once at top level. */
static t_execution_state	run_body(t_shell *state, t_ast_node *body)
{
	t_executable_node	body_exe;

	body_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, body, true);
	return (execute_tree_node(state, &body_exe));
}

static void	set_for_var(t_shell *state, char *name, char *val)
{
	t_env	*old;

	old = env_get(&state->env, name);
	if (old)
	{
		free(old->value);
		old->value = ft_strdup(val);
		return ;
	}
	env_set(&state->env, env_create(ft_strdup(name), ft_strdup(val), false));
}

static t_vec	expand_for_words(t_shell *state,
		t_ast_node *node, size_t wc)
{
	t_vec	words;
	size_t	i;

	vec_init(&words);
	words.elem_size = sizeof(char *);
	i = 0;
	while (i < wc)
	{
		expand_word_ro(state, vec_idx(&node->children, i), &words, false);
		i++;
	}
	return (words);
}

static t_execution_state	for_word_loop(t_shell *state,
		t_ast_node *node, char *var_name, size_t wc)
{
	t_execution_state	status;
	t_vec				words;
	size_t				i;

	words = expand_for_words(state, node, wc);
	status = res_status(0);
	i = 0;
	state->loop_depth++;
	while (i < words.len)
	{
		set_for_var(state, var_name, ((char **)words.ctx)[i]);
		status = run_body(state, vec_idx(&node->children, wc));
		if (handle_loop_ctl(state))
			break ;
		i++;
	}
	state->loop_depth--;
	i = 0;
	while (i < words.len)
		free(((char **)words.ctx)[i++]);
	free(words.ctx);
	return (status);
}

/*
** for NAME [in wordlist ;] do compound_list done
** children = [word_list...] + compound_list(body)
*/
/* `for NAME; do ... done` with no `in` list iterates over "$@" (POSIX). The
   parser sets node->negate when an `in` clause is present, so word_count==0 with
   no `in` means "iterate the positional parameters" (vs an explicit empty list). */
static t_execution_state	for_positional_loop(t_shell *state,
		t_ast_node *node, char *var_name)
{
	t_execution_state	status;
	char				*key;
	int					i;
	int					n;

	status = res_status(0);
	n = ft_atoi(env_expand(state, "#"));
	state->loop_depth++;
	i = 0;
	while (++i <= n)
	{
		key = ft_itoa(i);
		set_for_var(state, var_name, env_expand(state, key));
		free(key);
		status = run_body(state, vec_idx(&node->children, 0));
		if (handle_loop_ctl(state))
			break ;
	}
	state->loop_depth--;
	return (status);
}

t_execution_state	execute_for(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	char				*var_name;
	size_t				word_count;

	ft_assert(exe->node->children.len >= 1);
	var_name = ft_strndup(exe->node->token.start, exe->node->token.len);
	word_count = exe->node->children.len - 1;
	if (word_count == 0 && !exe->node->negate)
		status = for_positional_loop(state, exe->node, var_name);
	else
		status = for_word_loop(state, exe->node, var_name, word_count);
	free(var_name);
	return (status);
}

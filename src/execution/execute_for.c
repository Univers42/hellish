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

/* Expand all word-list children of the for node (indices 0..wc-1) into a
   flat list of strings, with field splitting and glob expansion.  We push
   a fresh word-slab frame first (word_slab_push) so the expanded strings
   are allocated there and released in one shot with a matching pop, which
   avoids any per-word xfree in for_word_loop.  The returned vec's strings
   still need individual xfree because they were strdup'd by expand_word_ro
   from the slab. */
static t_vec	expand_for_words(t_shell *state,
		t_ast_node *node, size_t wc)
{
	t_vec	words;
	size_t	i;
	int		o;

	o = word_slab_push(0);
	vec_init(&words);
	words.elem_size = sizeof(char *);
	i = 0;
	while (i < wc)
	{
		expand_word_ro(state, vec_idx(&node->children, i), &words, false);
		i++;
	}
	word_slab_push(o);
	return (words);
}

/* Run the body once per expanded word.  set_for_var updates the loop
   variable in the environment before each iteration.  After the loop the

   `step` (for_stride) is what makes zsh's `for a b (w x y z)` consume the
   list two at a time; it is 1 for every POSIX loop, which keeps the
   single-name path exactly as it was -- one env write per turn and no
   per-iteration allocation. */
static t_execution_state	for_word_loop(t_shell *state,
		t_ast_node *node, char *var_name, size_t wc)
{
	t_execution_state	status;
	t_vec				words;
	size_t				i[2];

	words = expand_for_words(state, node, wc);
	status = res_status(0);
	i[1] = for_stride(node);
	i[0] = 0;
	state->loop_depth++;
	while (i[0] < words.len)
	{
		fire_debug_trap(state);
		if (i[1] == 1)
			set_for_var(state, var_name, ((char **)words.ctx)[i[0]]);
		else
			zfor_bind_row(state, node, &words, i[0]);
		status = run_body(state, vec_idx(&node->children, wc));
		if (handle_loop_ctl(state))
			break ;
		i[0] += i[1];
	}
	state->loop_depth--;
	free_word_vec(&words);
	return (status);
}

/* `for NAME; do ... done` with no `in` iterates over "$@" (POSIX).  The
   parser sets node->negate when an explicit `in` clause was parsed, so
   word_count==0 AND !node->negate is the reliable signal for "iterate
   positional parameters" (rather than an explicit empty word list). */
static t_execution_state	for_positional_loop(t_shell *state,
		t_ast_node *node, char *var_name)
{
	t_execution_state	status;
	t_vec				words;
	t_vec				*prev;
	size_t				i;

	status = res_status(0);
	snapshot_positionals(state, &words);
	prev = state->for_snapshot;
	state->for_snapshot = &words;
	state->loop_depth++;
	i = 0;
	while (i < words.len)
	{
		fire_debug_trap(state);
		set_for_var(state, var_name, ((char **)words.ctx)[i]);
		i++;
		status = run_body(state, vec_idx(&node->children, 0));
		if (handle_loop_ctl(state))
			break ;
	}
	state->loop_depth--;
	state->for_snapshot = prev;
	free_positional_snapshot(&words);
	return (status);
}

/* for NAME [in wordlist]; do body; done.  The AST stores the variable
   name in the node's token; children are [word...body].  word_count is
   children.len-1 (last child is always the body).  Two dispatch paths:
   no `in` clause -> positional params ($@), else -> explicit word list. */
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
	xfree(var_name);
	return (status);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_function.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 17:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/*
** Look up a function by name in the shell's function table.
** Returns pointer to the function entry, or NULL.
*/
t_shell_func	*func_lookup(t_shell *state, const char *name)
{
	size_t			i;
	t_shell_func	*fn;

	i = 0;
	while (i < state->functions.len)
	{
		fn = (t_shell_func *)vec_idx(&state->functions, i);
		if (ft_strcmp(fn->name, name) == 0)
			return (fn);
		i++;
	}
	return (NULL);
}

/* unset -f name: remove a stored function definition (POSIX).
** No-op if absent. */
void	unset_function(t_shell *state, const char *name)
{
	t_shell_func	*arr;
	size_t			i;

	arr = (t_shell_func *)state->functions.ctx;
	i = 0;
	while (i < state->functions.len)
	{
		if (ft_strcmp(arr[i].name, name) == 0)
		{
			free(arr[i].name);
			free_ast(&arr[i].body);
			while (i + 1 < state->functions.len)
			{
				arr[i] = arr[i + 1];
				i++;
			}
			state->functions.len--;
			return ;
		}
		i++;
	}
}

/*
** Store (or overwrite) a function definition.
** Clones the body AST so the original can be freed.
*/
static void	store_function(t_shell *state, char *name, t_ast_node *body)
{
	t_shell_func	*existing;
	t_shell_func	new_fn;

	existing = func_lookup(state, name);
	if (existing)
	{
		free_ast(&existing->body);
		existing->body = deep_clone_ast(body);
		return ;
	}
	new_fn.name = ft_strdup(name);
	new_fn.body = deep_clone_ast(body);
	vec_push(&state->functions, &new_fn);
}

/*
** Execute a function definition: store the body in the function table.
** AST_FUNCTION_DEF: token = name, children[0] = body
*/
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe)
{
	char		*name;
	t_ast_node	*body;

	name = ft_strndup(exe->node->token.start, exe->node->token.len);
	body = vec_idx(&exe->node->children, 0);
	store_function(state, name, body);
	free(name);
	return (res_status(0));
}

/*
** Execute a function call: give it its own positional params by swapping in a
** freshly-built t_pos (saving the caller's by value — an O(1) struct copy, no
** env mutation), run the body IN PLACE, then restore the scope (`local`
** variables via scope_leave, positional params by restoring the saved struct).
**
** The body is run directly off fn->body (no per-call clone) — exactly as
** execute_while/execute_for run loop bodies in place. Word expansion is
** non-destructive (expand_word_ro clones each word it touches), so it never
** mutates the shared body AST, which makes repeated and recursive calls safe.
** Redirects re-resolve every call (commit_redir for non-heredocs, fresh
** materialize_heredoc for heredocs), so the per-node redir cache is never read
** stale across calls.
*/
t_execution_state	execute_func_call(t_shell *state, t_shell_func *fn,
						t_vec *argv)
{
	t_executable_node	body_exe;
	t_execution_state	status;
	t_pos				saved;
	t_ast_node			body;

	state->func_depth++;
	saved = state->pos;
	pos_borrow(&state->pos, (char **)argv->ctx + 1, argv->len - 1);
	body = fn->body;
	body_exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, &body, true);
	status = execute_tree_node(state, &body_exe);
	state->func_return = 0;
	scope_leave(state);
	pos_free(&state->pos);
	state->pos = saved;
	return (status);
}

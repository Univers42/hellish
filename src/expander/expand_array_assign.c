/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_assign.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

void	scope_save(t_shell *state, const char *key);

/* Apply an AST_ARRAY_ASSIGN node: arr=(a b c) rebuilds the array from
   scratch, arr+=(d e) keeps the existing records and appends after the
   highest index. Each element word SPLITS and GLOBS, like a command word
   and like bash: arr=($V) is one element per field, files=(*.c) one per
   match. This used to use assignment semantics (one field, no glob), which
   silently produced a one-element array holding the unsplit string.
   The result rides the normal pre_assigns path, so `arr=(1 2) cmd` scopes
   exactly like VAR=v cmd. */

/* Expand children [1..] (the element words) into heap strings. */
static void	expand_elems(t_shell *state, t_ast_node *node, t_vec *args)
{
	size_t	i;

	i = 1;
	while (i < node->children.len)
	{
		expand_word_elem_ro(state,
			&((t_ast_node *)node->children.ctx)[i], args);
		i++;
	}
}

/* Free the element strings and the vector itself. */
static void	free_elems(t_vec *args)
{
	size_t	i;

	i = 0;
	while (i < args->len)
		xfree(((char **)args->ctx)[i++]);
	xfree(args->ctx);
}

/* Is the command word an assignment builtin (local/declare/typeset/
   export/readonly)? For those, name=(...) persists in the right scope
   instead of being a temporary command prefix. */
int	is_assign_builtin(t_executable_cmd *ret)
{
	char	*c;

	if (ret->argv.len == 0 || !((char **)ret->argv.ctx)[0])
		return (0);
	c = ((char **)ret->argv.ctx)[0];
	return (!ft_strcmp(c, "local") || !ft_strcmp(c, "declare")
		|| !ft_strcmp(c, "typeset"));
}

/* Apply the array assignment persistently: local also snapshots the old
   value so the function-return path restores it. */
static void	persist_array(t_shell *state, t_executable_cmd *ret, t_env ev)
{
	if (!ft_strcmp(((char **)ret->argv.ctx)[0], "local")
		&& state->func_depth > 0)
		scope_save(state, ev.key);
	env_set(&state->env, ev);
}

int	handle_array_assign(t_shell *state, t_expander_simple_cmd *exp,
		t_executable_cmd *ret)
{
	t_vec			args;
	t_token			key;
	t_env			ev;
	t_arr_assign	aa;

	key = ((t_ast_node *)exp->curr->children.ctx)[0].token;
	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_elems(state, exp->curr, &args);
	aa.append = (key.len >= 2 && key.start[key.len - 2] == '+');
	ev.key = ft_strndup(key.start, key.len - 1 - aa.append);
	ev.exported = state->opt_allexport;
	aa.ev = &ev;
	aa.args = &args;
	ev.value = build_array_value(state, ret, &aa);
	if (is_assign_builtin(ret))
		persist_array(state, ret, ev);
	else
		vec_push(&ret->pre_assigns, &ev);
	return (free_elems(&args), 0);
}

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

/* Apply an AST_ARRAY_ASSIGN node: arr=(a b c) rebuilds the array from
   scratch, arr+=(d e) keeps the existing records and appends after the
   highest index. Each element word expands with assignment semantics
   (one field, no glob) — element-level splitting/globbing is a
   documented v1 divergence from bash. The result rides the normal
   pre_assigns path, so `arr=(1 2) cmd` scopes exactly like VAR=v cmd. */

/* Expand children [1..] (the element words) into heap strings. */
static void	expand_elems(t_shell *state, t_ast_node *node, t_vec *args)
{
	size_t	i;

	i = 1;
	while (i < node->children.len)
	{
		expand_word_assign_ro(state,
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

int	handle_array_assign(t_shell *state, t_expander_simple_cmd *exp,
		t_executable_cmd *ret)
{
	t_vec	args;
	t_token	key;
	t_env	ev;
	int		append;

	key = ((t_ast_node *)exp->curr->children.ctx)[0].token;
	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_elems(state, exp->curr, &args);
	append = (key.len >= 2 && key.start[key.len - 2] == '+');
	ev.key = ft_strndup(key.start, key.len - 1 - append);
	ev.exported = state->opt_allexport;
	if (append)
		ev.value = arr_from_elems((char **)args.ctx, (int)args.len,
				env_expand(state, ev.key));
	else
		ev.value = arr_from_elems((char **)args.ctx, (int)args.len, NULL);
	vec_push(&ret->pre_assigns, &ev);
	return (free_elems(&args), 0);
}

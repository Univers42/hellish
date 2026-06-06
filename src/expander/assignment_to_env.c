/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_to_env.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

/* Copy the assignment LHS (variable name) out of the first child token of
   an AST_ASSIGNMENT_WORD node.  The first child is always a TT_WORD whose
   content is the bare name without the trailing '='. */
static char	*dup_key_from_node(t_ast_node *node)
{
	return (ft_strndup(
			((t_ast_node *)node->children.ctx)[0].token.start,
		((t_ast_node *)node->children.ctx)[0].token.len
	));
}

/* Take the single expanded value string out of `args` (ownership transfer).
   The caller's expand_word_assign_ro is expected to have produced exactly
   one field (keep-as-one + no-glob).  The ft_assert(args->len == 1) fires
   in debug builds if that contract is violated. */
static char	*dup_value_from_args(t_vec *args)
{
	char	*val;

	if (!args->len)
		return (NULL);
	ft_assert(args->len == 1);
	val = ((char **)args->ctx)[0];
	if (val)
		return (val);
	return (ft_strdup(""));
}

/* Convert a VAR=value AST node into a t_env ready to be pushed into the
   environment.  The RHS (child [1]) is expanded with assignment semantics
   (keep-as-one, no glob, malloc allocator rather than slab) so the resulting
   value is safe to store long-term.  The opt_allexport flag pre-marks the
   env entry as exported if `set -a` is active. */
t_env	assignment_to_env(t_shell *state, t_ast_node *node)
{
	t_vec	args;
	t_env	ret;
	char	*v;

	ret = (t_env){.exported = state->opt_allexport};
	vec_init(&args);
	args.elem_size = sizeof(char *);
	ft_assert(node->children.len == 2);
	expand_word_assign_ro(state,
		&((t_ast_node *)node->children.ctx)[1], &args);
	ret.key = dup_key_from_node(node);
	if (args.len)
	{
		v = dup_value_from_args(&args);
		if (v)
			ret.value = v;
		else
			ret.value = ft_strdup("");
	}
	return (xfree(args.ctx), ret);
}

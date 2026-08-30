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
#include "env.h"
#include "arith.h"

void	apply_var_attrs(t_shell *state, t_env *ret);
void	scalar_append(t_shell *state, t_env *ret);

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

/* Turn an already-word-expanded subscript string into the 0-based index the
   store uses: the text is a bare arithmetic expression (variables and
   $((...)) are gone), so arith_expand handles it directly, and sub_to_index
   applies the dialect's counting base and wraps negatives against `count`.

   `a[-1]=x` means the LAST element in both dialects.  Without the wrap it
   was stored at index -1, which no read can reach and which "${a[@]}" then
   printed ahead of the real elements -- silently, as `-1x`. */
static long	arr_sub_index(t_shell *state, const char *sub, long count)
{
	char	*res;
	long	idx;

	if (!sub || !*sub)
		return (0);
	res = arith_expand(state, sub, (int)ft_strlen(sub));
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	return (xfree(res), sub_to_index(state, idx, count));
}

/* arr[expr]=v: the key still carries its "[expr]" suffix here. Evaluate
   the subscript arithmetically, rebuild the array value with that
   element set (scalars promote to a one-element array first), truncate
   the key to the bare name. A plain key passes through untouched.

   The slice check comes before the index one, because `lo,hi` evaluates
   perfectly well as arithmetic -- the comma operator yields `hi` -- and
   would write ONE element where zsh replaces a whole run. */
static void	subscript_assign(t_shell *state, t_env *ret)
{
	char	*br;
	char	*res;
	char	*old;
	t_slice	r;

	br = ft_strchr(ret->key, '[');
	if (!br || !ret->value)
		return ;
	*br = '\0';
	old = env_expand(state, ret->key);
	if (assoc_is(old))
		return (assoc_elem_assign(state, ret, br + 1, old));
	res = expand_param_word(state, br + 1, (int)ft_strlen(br + 1) - 1, false);
	r = zsh_slice_bounds(state, res, (int)ft_strlen(res), old);
	if (r.lo != SLICE_NONE)
		return (xfree(res), zsh_slice_set(ret, old, r));
	r.lo = arr_sub_index(state, res, arr_count(old));
	if (r.lo < 0)
		return ((void)(bad_subscript(state, ret->key, res), xfree(res)));
	xfree(res);
	res = arr_with_set(old, r.lo, ret->value);
	xfree(ret->value);
	ret->value = res;
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
	apply_var_attrs(state, &ret);
	scalar_append(state, &ret);
	subscript_assign(state, &ret);
	return (xfree(args.ctx), ret);
}
